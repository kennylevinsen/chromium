// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.hostimpl.storage;

import static com.google.android.libraries.feed.api.host.storage.JournalOperation.Type.APPEND;
import static com.google.android.libraries.feed.api.host.storage.JournalOperation.Type.COPY;
import static com.google.android.libraries.feed.api.host.storage.JournalOperation.Type.DELETE;

import android.content.Context;
import android.support.annotation.VisibleForTesting;
import com.google.android.libraries.feed.api.host.storage.CommitResult;
import com.google.android.libraries.feed.api.host.storage.JournalMutation;
import com.google.android.libraries.feed.api.host.storage.JournalOperation;
import com.google.android.libraries.feed.api.host.storage.JournalOperation.Append;
import com.google.android.libraries.feed.api.host.storage.JournalOperation.Copy;
import com.google.android.libraries.feed.api.host.storage.JournalStorage;
import com.google.android.libraries.feed.api.host.storage.JournalStorageDirect;
import com.google.android.libraries.feed.api.internal.common.ThreadUtils;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.logging.Logger;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.Executor;

/**
 * Implementation of {@link JournalStorage} that persists data to disk.
 *
 * <p>Data is stored in files, with each append consisting of first the size of the bytes to write,
 * followed by the bytes themselves. For example, for a byte array of size 4, a total of 8 bytes
 * will be written: the 4 bytes representing the integer 4 (size), followed by the 4 bytes for the
 * append.
 */
public final class PersistentJournalStorage implements JournalStorage, JournalStorageDirect {

  private static final String TAG = "PersistentJournal";
  /** The schema version currently in use. */
  private static final int SCHEMA_VERSION = 1;

  private static final String SHARED_PREFERENCES = "JOURNAL_SP";
  private static final String SCHEMA_KEY = "JOURNAL_SCHEMA";

  // Optional Content directory - By default this is not used
  private static final String JOURNAL_DIR = "journal";
  private static final String ENCODING = "UTF-8";
  private static final int INTEGER_BYTE_SIZE = 4;
  private static final String ASTERISK = "_ATK_";
  private static final int MAX_BYTE_SIZE = 1000000;

  private final Context context;
  private final ThreadUtils threadUtils;
  private final Executor executor;
  /*@Nullable*/ private final String persistenceDir;
  private File journalDir;

  /**
   * The schema of existing content. If this does not match {@code SCHEMA_VERSION}, all existing
   * content will be wiped so there are no version mismatches where data cannot be read / written
   * correctly.
   */
  private int existingSchema;

  public PersistentJournalStorage(
      Context context,
      Executor executorService,
      ThreadUtils threadUtils,
      /*@Nullable*/ String persistenceDir) {
    this.context = context;
    this.executor = executorService;
    this.threadUtils = threadUtils;
    // TODO: See https://goto.google.com/tiktok-conformance-violations/SHARED_PREFS
    this.existingSchema =
        context
            .getSharedPreferences(SHARED_PREFERENCES, Context.MODE_PRIVATE)
            .getInt(SCHEMA_KEY, 0);
    this.persistenceDir = persistenceDir;
  }

  @Override
  public void read(String journalName, Consumer<Result<List<byte[]>>> consumer) {
    threadUtils.checkMainThread();
    executor.execute(() -> consumer.accept(read(journalName)));
  }

  @Override
  public Result<List<byte[]>> read(String journalName) {
    initializeJournalDir();

    String sanitizedJournalName = sanitize(journalName);
    if (!sanitizedJournalName.isEmpty()) {
      File journal = new File(journalDir, sanitizedJournalName);
      try {
        return Result.success(getJournalContents(journal));
      } catch (IOException e) {
        Logger.e(TAG, "Error occured reading journal %s", journalName);
        return Result.failure();
      }
    }
    return Result.failure();
  }

  private List<byte[]> getJournalContents(File journal) throws IOException {
    threadUtils.checkNotMainThread();

    List<byte[]> journalContents = new ArrayList<>();
    if (journal.exists()) {
      // Read byte size & bytes for each entry in the journal. See class comment for more info on
      // format.
      try (FileInputStream inputStream = new FileInputStream(journal)) {
        byte[] lengthBytes = new byte[INTEGER_BYTE_SIZE];
        while (inputStream.available() > 0) {
          readBytes(inputStream, lengthBytes, INTEGER_BYTE_SIZE);
          int size = ByteBuffer.wrap(lengthBytes).getInt();
          if (size > MAX_BYTE_SIZE || size < 0) {
            throw new IOException(String.format(Locale.US, "Unexpected byte size %d", size));
          }

          byte[] contentBytes = new byte[size];
          readBytes(inputStream, contentBytes, size);
          journalContents.add(contentBytes);
        }
      } catch (IOException e) {
        Logger.e(TAG, "Error reading file", e);
        throw new IOException("Error reading journal file", e);
      }
    }
    return journalContents;
  }

  private void readBytes(FileInputStream inputStream, byte[] dest, int size) throws IOException {
    int bytesRead = inputStream.read(dest, 0, size);
    if (bytesRead != size) {
      throw new IOException(
          String.format(
              Locale.US, "Expected to read %d bytes, but read %d bytes", size, bytesRead));
    }
  }

  @Override
  public void commit(JournalMutation mutation, Consumer<CommitResult> consumer) {
    threadUtils.checkMainThread();
    executor.execute(() -> consumer.accept(commit(mutation)));
  }

  @Override
  public CommitResult commit(JournalMutation mutation) {
    initializeJournalDir();

    String sanitizedJournalName = sanitize(mutation.getJournalName());
    if (!sanitizedJournalName.isEmpty()) {
      File journal = new File(journalDir, sanitizedJournalName);

      for (JournalOperation operation : mutation.getOperations()) {
        if (operation.getType() == APPEND) {
          if (!append((Append) operation, journal)) {
            return CommitResult.FAILURE;
          }
        } else if (operation.getType() == COPY) {
          if (!copy((Copy) operation, journal)) {
            return CommitResult.FAILURE;
          }
        } else if (operation.getType() == DELETE) {
          if (!delete(journal)) {
            return CommitResult.FAILURE;
          }
        } else {
          Logger.e(TAG, "Unrecognized journal operation type %s", operation.getType());
        }
      }

      return CommitResult.SUCCESS;
    }
    return CommitResult.FAILURE;
  }

  @Override
  public void deleteAll(Consumer<CommitResult> consumer) {
    threadUtils.checkMainThread();

    executor.execute(() -> consumer.accept(deleteAllInitialized()));
  }

  @Override
  public CommitResult deleteAll() {
    initializeJournalDir();
    return deleteAllInitialized();
  }

  private CommitResult deleteAllInitialized() {
    boolean success = true;

    File[] files = journalDir.listFiles();
    if (files != null) {
      // Delete all files in the journal directory
      for (File file : files) {
        if (!file.delete()) {
          Logger.e(
              TAG, "Error deleting file when deleting all journals for file %s", file.getName());
          success = false;
        }
      }
    }
    success &= journalDir.delete();
    return success ? CommitResult.SUCCESS : CommitResult.FAILURE;
  }

  private boolean delete(File journal) {
    threadUtils.checkNotMainThread();

    if (!journal.exists()) {
      // If the file doesn't exist, let's call it deleted.
      return true;
    }
    boolean result = journal.delete();
    if (!result) {
      Logger.e(TAG, "Error deleting journal %s", journal.getName());
    }
    return result;
  }

  private boolean copy(Copy operation, File journal) {
    threadUtils.checkNotMainThread();

    try {
      if (!journal.exists()) {
        Logger.w(TAG, "Journal file %s does not exist, creating empty version", journal.getName());
        if (!journal.createNewFile()) {
          Logger.e(TAG, "Journal file %s exists while trying to create it", journal.getName());
        }
      }
      String sanitizedDestJournalName = sanitize(operation.getToJournalName());
      if (!sanitizedDestJournalName.isEmpty()) {
        copyFile(journal, sanitizedDestJournalName);
        return true;
      }
    } catch (IOException e) {
      Logger.e(
          TAG,
          e,
          "Error copying journal %s to %s",
          journal.getName(),
          operation.getToJournalName());
    }
    return false;
  }

  private void copyFile(File journal, String destinationFileName) throws IOException {
    InputStream inputStream = null;
    OutputStream outputStream = null;
    try {
      File destination = new File(journalDir, destinationFileName);
      inputStream = new FileInputStream(journal);
      outputStream = new FileOutputStream(destination);
      byte[] bytes = new byte[512];
      int length;
      while ((length = inputStream.read(bytes)) > 0) {
        outputStream.write(bytes, 0, length);
      }
    } finally {
      if (inputStream != null) {
        inputStream.close();
      }
      if (outputStream != null) {
        outputStream.close();
      }
    }
  }

  private boolean append(Append operation, File journal) {
    threadUtils.checkNotMainThread();

    if (!journal.exists()) {
      try {
        journal.createNewFile();
      } catch (IOException e) {
        Logger.e(TAG, "Could not create file to append to for journal %s.", journal.getName());
        return false;
      }
    }

    byte[] journalBytes = operation.getValue();
    byte[] sizeBytes = ByteBuffer.allocate(INTEGER_BYTE_SIZE).putInt(journalBytes.length).array();
    return writeBytes(journal, sizeBytes, journalBytes);
  }

  /** Write byte size & bytes into the journal. See class comment for more info on format. */
  private boolean writeBytes(File journal, byte[] sizeBytes, byte[] journalBytes) {
    try (FileOutputStream out = new FileOutputStream(journal, /* append= */ true)) {
      out.write(sizeBytes);
      out.write(journalBytes);
      return true;
    } catch (IOException e) {
      Logger.e(
          TAG,
          "Error appending byte[] %s (size byte[] %s) for journal %s",
          journalBytes,
          sizeBytes,
          journal.getName());
      return false;
    }
  }

  @Override
  public void exists(String journalName, Consumer<Result<Boolean>> consumer) {
    threadUtils.checkMainThread();
    executor.execute(() -> consumer.accept(exists(journalName)));
  }

  @Override
  public Result<Boolean> exists(String journalName) {
    initializeJournalDir();

    String sanitizedJournalName = sanitize(journalName);
    if (!sanitizedJournalName.isEmpty()) {
      File journal = new File(journalDir, sanitizedJournalName);
      return Result.success(journal.exists());
    }
    return Result.failure();
  }

  @Override
  public void getAllJournals(Consumer<Result<List<String>>> consumer) {
    threadUtils.checkMainThread();
    executor.execute(() -> consumer.accept(getAllJournals()));
  }

  @Override
  public Result<List<String>> getAllJournals() {
    initializeJournalDir();

    File[] files = journalDir.listFiles();
    List<String> journals = new ArrayList<>();
    if (files != null) {
      for (File file : files) {
        String desanitizedFileName = desanitize(file.getName());
        if (!desanitizedFileName.isEmpty()) {
          journals.add(desanitizedFileName);
        }
      }
    }
    return Result.success(journals);
  }

  private void initializeJournalDir() {
    // if we've set the journalDir then just verify that it exists
    if (journalDir != null) {
      if (!journalDir.exists()) {
        if (!journalDir.mkdir()) {
          Logger.w(TAG, "Jardin journal directory already exists");
        }
      }
      return;
    }

    // Create the root directory persistent files
    if (persistenceDir != null) {
      File persistenceRoot = context.getDir(persistenceDir, Context.MODE_PRIVATE);
      if (!persistenceRoot.exists()) {
        if (!persistenceRoot.mkdir()) {
          Logger.w(TAG, "persistenceDir directory already exists");
        }
      }
      journalDir = new File(persistenceRoot, JOURNAL_DIR);
    } else {
      journalDir = context.getDir(JOURNAL_DIR, Context.MODE_PRIVATE);
    }
    if (existingSchema != SCHEMA_VERSION) {
      // For schema mismatch, delete everything.
      CommitResult result = deleteAllInitialized();
      if (result == CommitResult.SUCCESS
          && context
              .getSharedPreferences(SHARED_PREFERENCES, Context.MODE_PRIVATE)
              .edit()
              .putInt(SCHEMA_KEY, SCHEMA_VERSION)
              .commit()) {
        existingSchema = SCHEMA_VERSION;
      }
    }
    if (!journalDir.exists()) {
      if (!journalDir.mkdir()) {
        Logger.w(TAG, "journal directory already exists");
      }
    }
  }

  @VisibleForTesting
  String sanitize(String journalName) {
    try {
      // * is not replaced by URL encoder
      String sanitized = URLEncoder.encode(journalName, ENCODING);
      return sanitized.replace("*", ASTERISK);
    } catch (UnsupportedEncodingException e) {
      // Should not happen
      Logger.e(TAG, "Error sanitizing journal name %s", journalName);
      return "";
    }
  }

  @VisibleForTesting
  String desanitize(String sanitizedJournalName) {
    try {
      // * is not replaced by URL encoder
      String desanitized = URLDecoder.decode(sanitizedJournalName, ENCODING);
      return desanitized.replace(ASTERISK, "*");
    } catch (UnsupportedEncodingException e) {
      // Should not happen
      Logger.e(TAG, "Error desanitizing journal name %s", sanitizedJournalName);
      return "";
    }
  }
}

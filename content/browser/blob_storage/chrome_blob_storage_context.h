// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_CHROME_BLOB_STORAGE_CONTEXT_H_
#define CONTENT_BROWSER_BLOB_STORAGE_CHROME_BLOB_STORAGE_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom.h"

namespace base {
class TaskRunner;
}

namespace storage {
class BlobStorageContext;
namespace mojom {
class BlobStorageContext;
}
}

namespace content {
class BlobHandle;
class BrowserContext;

// A context class that keeps track of BlobStorageController used by the chrome.
// There is an instance associated with each BrowserContext. There could be
// multiple URLRequestContexts in the same browser context that refers to the
// same instance.
//
// All methods, except the ctor, are expected to be called on
// the IO thread (unless specifically called out in doc comments).
class CONTENT_EXPORT ChromeBlobStorageContext
    : public base::RefCountedThreadSafe<ChromeBlobStorageContext,
                                        BrowserThread::DeleteOnIOThread> {
 public:
  ChromeBlobStorageContext();

  // Must be called on the UI thread.
  static ChromeBlobStorageContext* GetFor(
      BrowserContext* browser_context);

  void InitializeOnIOThread(base::FilePath blob_storage_dir,
                            scoped_refptr<base::TaskRunner> file_task_runner);

  storage::BlobStorageContext* context() const;

  // Return a BlobStorageContext mojo interface to be used by storage apis.
  // This interface should not be exposed to renderers.
  mojo::PendingRemote<storage::mojom::BlobStorageContext> MojoContext() const;

  // Returns a NULL scoped_ptr on failure.
  std::unique_ptr<BlobHandle> CreateMemoryBackedBlob(
      base::span<const uint8_t> data,
      const std::string& content_type);

  // Returns a SharedURLLoaderFactory capable of creating URLLoaders for exactly
  // the one URL associated with the passed in |token|. Attempting to load any
  // other URL through the factory will result in an error. If the |token|
  // itself is invalid all requests will result in errors.
  // Must be called on the UI thread.
  static scoped_refptr<network::SharedURLLoaderFactory>
  URLLoaderFactoryForToken(
      BrowserContext* browser_context,
      mojo::PendingRemote<blink::mojom::BlobURLToken> token);

  // Similar to the above method this also returns a factory capable of loading
  // a single (blob) URL. If the |url| isn't a valid/registered blob URL at the
  // time this method is called, using the resulting factory will always result
  // in an error.
  // Generally you should prefer using the above method and pass around a
  // BlobURLToken rather than a blob URL. This is because the BlobURLToken will
  // ensure that the URL and the blob it refers to stay alive, while merely
  // holding on to the URL has no such guarantees.
  // Must be called on the UI thread.
  static scoped_refptr<network::SharedURLLoaderFactory> URLLoaderFactoryForUrl(
      BrowserContext* browser_context,
      const GURL& url);

  // Must be called on the UI thread.
  static mojo::PendingRemote<blink::mojom::Blob> GetBlobRemote(
      BrowserContext* browser_context,
      const std::string& uuid);

 protected:
  virtual ~ChromeBlobStorageContext();

 private:
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  friend class base::DeleteHelper<ChromeBlobStorageContext>;

  std::unique_ptr<storage::BlobStorageContext> context_;
};

// Returns the BlobStorageContext associated with the
// ChromeBlobStorageContext instance passed in.
storage::BlobStorageContext* GetBlobStorageContext(
    ChromeBlobStorageContext* blob_storage_context);

extern const char kBlobStorageContextKeyName[];

}  // namespace content

#endif  // CONTENT_BROWSER_BLOB_STORAGE_CHROME_BLOB_STORAGE_CONTEXT_H_

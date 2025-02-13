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

package com.google.android.libraries.feed.hostimpl.scheduler;

import com.google.android.libraries.feed.api.host.scheduler.SchedulerApi;
import com.google.android.libraries.feed.api.internal.common.ThreadUtils;
import com.google.android.libraries.feed.common.concurrent.MainThreadRunner;
import com.google.android.libraries.feed.common.concurrent.SimpleSettableFuture;
import com.google.android.libraries.feed.common.logging.Logger;
import java.util.concurrent.ExecutionException;

/** Wrapper class which will call the {@link SchedulerApi} on the main thread. */
public final class SchedulerApiWrapper implements SchedulerApi {
  private static final String TAG = "SchedulerApiWrapper";
  private final SchedulerApi directSchedulerApi;
  private final ThreadUtils threadUtils;
  private final MainThreadRunner mainThreadRunner;

  public SchedulerApiWrapper(
      SchedulerApi directSchedulerApi, ThreadUtils threadUtils, MainThreadRunner mainThreadRunner) {
    Logger.i(TAG, "Create SchedulerApiMainThreadWrapper");
    this.directSchedulerApi = directSchedulerApi;
    this.threadUtils = threadUtils;
    this.mainThreadRunner = mainThreadRunner;
  }

  @RequestBehavior
  @Override
  public int shouldSessionRequestData(SessionState sessionState) {
    if (threadUtils.isMainThread()) {
      return directSchedulerApi.shouldSessionRequestData(sessionState);
    }
    SimpleSettableFuture<Integer> future = new SimpleSettableFuture<>();
    mainThreadRunner.execute(
        TAG + " shouldSessionRequestData",
        () -> future.put(directSchedulerApi.shouldSessionRequestData(sessionState)));
    try {
      return future.get();
    } catch (InterruptedException | ExecutionException e) {
      Logger.e(TAG, e, null);
    }
    return RequestBehavior.NO_REQUEST_WITH_WAIT;
  }

  @Override
  public void onReceiveNewContent(long contentCreationDateTimeMs) {
    if (threadUtils.isMainThread()) {
      directSchedulerApi.onReceiveNewContent(contentCreationDateTimeMs);
      return;
    }
    mainThreadRunner.execute(
        TAG + " onReceiveNewContent",
        () -> {
          directSchedulerApi.onReceiveNewContent(contentCreationDateTimeMs);
        });
  }

  @Override
  public void onRequestError(int networkResponseCode) {
    if (threadUtils.isMainThread()) {
      directSchedulerApi.onRequestError(networkResponseCode);
      return;
    }
    mainThreadRunner.execute(
        TAG + " onRequestError",
        () -> {
          directSchedulerApi.onRequestError(networkResponseCode);
        });
  }
}

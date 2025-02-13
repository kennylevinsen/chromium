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

package com.google.android.libraries.feed.api.internal.scope;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.host.logging.RequestReason;
import com.google.android.libraries.feed.api.internal.lifecycle.Resettable;
import com.google.android.libraries.feed.api.internal.sessionmanager.FeedSessionManager;
import com.google.android.libraries.feed.common.concurrent.testing.FakeTaskQueue;
import com.google.android.libraries.feed.common.concurrent.testing.FakeThreadUtils;
import com.google.android.libraries.feed.common.time.testing.FakeClock;
import com.google.android.libraries.feed.feedapplifecyclelistener.FeedAppLifecycleListener;
import com.google.search.now.feed.client.StreamDataProto.UiContext;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link ClearAllListener} class. */
@RunWith(RobolectricTestRunner.class)
public class ClearAllListenerTest {
  private final FakeClock fakeClock = new FakeClock();
  private final FakeThreadUtils fakeThreadUtils = FakeThreadUtils.withThreadChecks();

  @Mock private Resettable store;
  @Mock private FeedSessionManager feedSessionManager;
  private FakeTaskQueue fakeTaskQueue;
  private FeedAppLifecycleListener appLifecycleListener;

  @Before
  public void setUp() {
    initMocks(this);
    fakeTaskQueue = new FakeTaskQueue(fakeClock, fakeThreadUtils);
    fakeTaskQueue.initialize(() -> {});
    appLifecycleListener = new FeedAppLifecycleListener(fakeThreadUtils);
  }

  @Test
  public void testClearAll() {
    setupClearAllListener();
    appLifecycleListener.onClearAll();
    verify(feedSessionManager).reset();
    verify(store).reset();
    assertThat(fakeTaskQueue.resetWasCalled()).isTrue();
    assertThat(fakeTaskQueue.completeResetWasCalled()).isTrue();
  }

  @Test
  public void testClearAllWithRefresh() {
    setupClearAllListener();
    appLifecycleListener.onClearAllWithRefresh();
    verify(feedSessionManager).reset();
    verify(feedSessionManager)
        .triggerRefresh(null, RequestReason.CLEAR_ALL, UiContext.getDefaultInstance());
    verify(store).reset();
    assertThat(fakeTaskQueue.resetWasCalled()).isTrue();
    assertThat(fakeTaskQueue.completeResetWasCalled()).isTrue();
  }

  @Test
  public void testNonClearLifecycld() {
    setupClearAllListener();
    appLifecycleListener.onEnterForeground();
    verifyZeroInteractions(feedSessionManager);
    verifyZeroInteractions(store);
  }

  private void setupClearAllListener() {
    new ClearAllListener(
        fakeTaskQueue, feedSessionManager, store, fakeThreadUtils, appLifecycleListener);
  }
}

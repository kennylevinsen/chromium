// Copyright 2019 The Feed Authors.
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

package com.google.android.libraries.feed.sharedstream.scroll;

import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.ViewTreeObserver.OnGlobalLayoutListener;
import com.google.android.libraries.feed.piet.FrameAdapter;
import com.google.android.libraries.feed.sharedstream.publicapi.scroll.ScrollObservable;
import com.google.android.libraries.feed.sharedstream.publicapi.scroll.ScrollObserver;

/** Scroll observer that triggers Piet scroll actions. */
public class PietScrollObserver implements ScrollObserver {

  private final FrameAdapter frameAdapter;
  private final View viewport;
  private final ScrollObservable scrollObservable;
  private final FirstDrawTrigger firstDrawTrigger;

  /**
   * Construct a PietScrollObserver.
   *
   * @param frameAdapter Piet FrameAdapter which will have view actions triggered by this scroll
   *     observer
   * @param viewport the view containing the frameAdapter, the bounds of which determine whether the
   *     frame is visible
   * @param scrollObservable used to get the scroll state to ensure scrolling is idle before
   *     triggering
   */
  @SuppressWarnings("initialization") // Doesn't like the OnAttachStateChangeListener.
  public PietScrollObserver(
      FrameAdapter frameAdapter, View viewport, ScrollObservable scrollObservable) {
    this.frameAdapter = frameAdapter;
    this.viewport = viewport;
    this.scrollObservable = scrollObservable;

    firstDrawTrigger = new FirstDrawTrigger();

    frameAdapter
        .getFrameContainer()
        .addOnAttachStateChangeListener(
            new OnAttachStateChangeListener() {
              @Override
              public void onViewAttachedToWindow(View view) {
                installFirstDrawTrigger();
              }

              @Override
              public void onViewDetachedFromWindow(View view) {
                uninstallFirstDrawTrigger();
              }
            });
  }

  @Override
  public void onScrollStateChanged(View view, String featureId, int newState, long timestamp) {
    // There was logic in here previously ensuring that the state had changed; however, you can
    // have a case where the feed is idle and you fling the feed, and it goes to idle again
    // without triggering the observer during scrolling. Just triggering this every time the feed
    // comes to rest appears to have the desired behavior. We can think about also triggering
    // while the feed is scrolling; not sure how frequently the observer triggers during scroll.
    if (newState == RecyclerView.SCROLL_STATE_IDLE) {
      frameAdapter.triggerViewActions(viewport);
    }
  }

  @Override
  public void onScroll(View view, String featureId, int dx, int dy) {
    // no-op
  }

  /** Install the first-draw triggering observer. */
  public void installFirstDrawTrigger() {
    frameAdapter
        .getFrameContainer()
        .getViewTreeObserver()
        .addOnGlobalLayoutListener(firstDrawTrigger);
  }

  /** Uninstall the first-draw triggering observer. */
  public void uninstallFirstDrawTrigger() {
    frameAdapter
        .getFrameContainer()
        .getViewTreeObserver()
        .removeOnGlobalLayoutListener(firstDrawTrigger);
  }

  class FirstDrawTrigger implements OnGlobalLayoutListener {
    @Override
    public void onGlobalLayout() {
      uninstallFirstDrawTrigger();

      if (scrollObservable.getCurrentScrollState() == RecyclerView.SCROLL_STATE_IDLE) {
        frameAdapter.triggerViewActions(viewport);
      }
    }
  }
}

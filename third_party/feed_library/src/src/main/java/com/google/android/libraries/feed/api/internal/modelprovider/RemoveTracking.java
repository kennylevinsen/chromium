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

package com.google.android.libraries.feed.api.internal.modelprovider;

import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.functional.Function;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import java.util.ArrayList;
import java.util.List;

/**
 * Class which implements the filtering portion of remove tracking. This class creates a {@code
 * List<T>} of items using the {@code filterPredicate} function to both filter and transform a
 * {@link StreamFeature}. The function may return null, these values will be filtered.
 *
 * <p>The {@link #triggerConsumerUpdate()} will call the {@link Consumer}. It will be called when
 * all remove within the ModelProvider mutation have been processed.
 *
 * <p>The {@code Consumer} will be called on the main thread and called once per mutation. In
 * addition, the {@code Consumer} is called before the change operation.
 */
public final class RemoveTracking<T> {
  private final Function<StreamFeature, /*@Nullable*/ T> filterPredicate;
  private final Consumer<List<T>> consumer;
  private final List<T> matchingItems = new ArrayList<>();

  /**
   * Create the state necessary to call transform and filter the removed subtree before calling the
   * {@link Consumer}.
   */
  public RemoveTracking(
      Function<StreamFeature, /*@Nullable*/ T> filterPredicate, Consumer<List<T>> consumer) {
    this.filterPredicate = filterPredicate;
    this.consumer = consumer;
  }

  /**
   * Called to transform and filter a {@link StreamFeature} found within a subtree being removed.
   */
  public void filterStreamFeature(StreamFeature streamFeature) {
    T value = filterPredicate.apply(streamFeature);
    if (value != null) {
      matchingItems.add(value);
    }
  }

  /** Called on the main thread call the {@link Consumer} after all removes have been processed. */
  public void triggerConsumerUpdate() {
    consumer.accept(matchingItems);
  }
}

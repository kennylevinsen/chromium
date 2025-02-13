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

package com.google.android.libraries.feed.feedstore.internal;

import static com.google.android.libraries.feed.feedstore.internal.FeedStoreConstants.SEMANTIC_PROPERTIES_PREFIX;
import static com.google.android.libraries.feed.feedstore.internal.FeedStoreConstants.SHARED_STATE_PREFIX;
import static com.google.android.libraries.feed.feedstore.internal.FeedStoreConstants.UPLOADABLE_ACTION_PREFIX;

import com.google.android.libraries.feed.api.host.config.Configuration;
import com.google.android.libraries.feed.api.host.config.Configuration.ConfigKey;
import com.google.android.libraries.feed.api.host.logging.Task;
import com.google.android.libraries.feed.api.host.storage.CommitResult;
import com.google.android.libraries.feed.api.host.storage.ContentMutation;
import com.google.android.libraries.feed.api.host.storage.ContentStorageDirect;
import com.google.android.libraries.feed.common.Result;
import com.google.android.libraries.feed.common.concurrent.TaskQueue;
import com.google.android.libraries.feed.common.concurrent.TaskQueue.TaskType;
import com.google.android.libraries.feed.common.functional.Supplier;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import com.google.search.now.feed.client.StreamDataProto.StreamLocalAction;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/** Storage Content Garbage Collector. */
public final class ContentGc {
  private static final String TAG = "ContentGc";

  private final Supplier<Set<String>> accessibleContentSupplier;
  private final Set<String> reservedContentIds;
  private final Supplier<Set<StreamLocalAction>> actionsSupplier;
  private final ContentStorageDirect contentStorageDirect;
  private final TimingUtils timingUtils;
  private final TaskQueue taskQueue;
  private final boolean keepSharedStates;
  private final long maxAllowedGcAttempts;

  private int contentGcAttempts = 0;

  ContentGc(
      Configuration configuration,
      Supplier<Set<String>> accessibleContentSupplier,
      Set<String> reservedContentIds,
      Supplier<Set<StreamLocalAction>> actionsSupplier,
      ContentStorageDirect contentStorageDirect,
      TaskQueue taskQueue,
      TimingUtils timingUtils,
      boolean keepSharedStates) {
    this.accessibleContentSupplier = accessibleContentSupplier;
    this.reservedContentIds = reservedContentIds;
    this.actionsSupplier = actionsSupplier;
    this.contentStorageDirect = contentStorageDirect;
    this.taskQueue = taskQueue;
    this.timingUtils = timingUtils;
    this.keepSharedStates = keepSharedStates;
    maxAllowedGcAttempts = configuration.getValueOrDefault(ConfigKey.MAXIMUM_GC_ATTEMPTS, 10L);
  }

  void gc() {
    if (taskQueue.hasBacklog() && contentGcAttempts < maxAllowedGcAttempts) {
      Logger.i(TAG, "Re-enqueuing triggerContentGc; attempts(%d)", contentGcAttempts);
      contentGcAttempts++;
      taskQueue.execute(Task.GARBAGE_COLLECT_CONTENT, TaskType.BACKGROUND, this::gc);
      return;
    }

    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> population = getPopulation();
    // remove the items in the population that are accessible, reserved, or semantic properties
    // either accessible or associated with an action
    Set<String> accessibleContent = getAccessible();
    population.removeAll(accessibleContent);
    population.removeAll(reservedContentIds);
    population.removeAll(getAccessibleSemanticProperties(accessibleContent));
    population.removeAll(getLocalActionSemanticProperties(getLocalActions()));
    filterUploadableActions(population);
    if (keepSharedStates) {
      filterSharedStates(population);
    } else {
      population.removeAll(getAccessibleSharedStates(accessibleContent));
    }

    // Population now contains only un-accessible items
    removeUnAccessible(population);
    tracker.stop("task", "ContentGc", "contentItemsRemoved", population.size());
  }

  private void removeUnAccessible(Set<String> unAccessible) {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    ContentMutation.Builder mutationBuilder = new ContentMutation.Builder();
    for (String key : unAccessible) {
      Logger.i(TAG, "Removing %s", key);
      mutationBuilder.delete(key);
    }
    CommitResult result = contentStorageDirect.commit(mutationBuilder.build());
    if (result == CommitResult.FAILURE) {
      Logger.e(TAG, "Content Modification failed removing unaccessible items.");
    }
    tracker.stop("", "removeUnAccessible", "mutations", unAccessible.size());
  }

  private void filterSharedStates(Set<String> population) {
    filterPrefix(population, SHARED_STATE_PREFIX);
  }

  private void filterUploadableActions(Set<String> population) {
    filterPrefix(population, UPLOADABLE_ACTION_PREFIX);
  }

  private void filterPrefix(Set<String> population, String prefix) {
    int size = population.size();
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Iterator<String> i = population.iterator();
    while (i.hasNext()) {
      String key = i.next();
      if (key.startsWith(prefix)) {
        i.remove();
      }
    }
    tracker.stop("", "filterPrefix " + prefix, population.size() - size);
  }

  private Set<String> getAccessible() {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> accessibleContent = accessibleContentSupplier.get();
    tracker.stop("", "getAccessible", "accessableContent", accessibleContent.size());
    return accessibleContent;
  }

  private Set<StreamLocalAction> getLocalActions() {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<StreamLocalAction> actions = actionsSupplier.get();
    tracker.stop("", "getLocalActions", "actionCount", actions.size());
    return actions;
  }

  private Set<String> getPopulation() {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> population = new HashSet<>();
    Result<List<String>> result = contentStorageDirect.getAllKeys();
    if (result.isSuccessful()) {
      population.addAll(result.getValue());
    } else {
      Logger.e(TAG, "Unable to get all content, getAll failed");
    }
    tracker.stop("", "getPopulation", "contentPopulation", population.size());
    return population;
  }

  private Set<String> getAccessibleSemanticProperties(Set<String> accessibleContent) {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> semanticPropertiesKeys = new HashSet<>();
    for (String accessibleContentId : accessibleContent) {
      String semanticPropertyKey = SEMANTIC_PROPERTIES_PREFIX + accessibleContentId;
      semanticPropertiesKeys.add(semanticPropertyKey);
    }
    tracker.stop(
        "",
        "getAccessibleSemanticProperties",
        "accessibleSemanticPropertiesSize",
        semanticPropertiesKeys.size());
    return semanticPropertiesKeys;
  }

  private Set<String> getAccessibleSharedStates(Set<String> accessibleContent) {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> sharedStateKeys = new HashSet<>();
    for (String accessibleContentId : accessibleContent) {
      String sharedStateKey = SHARED_STATE_PREFIX + accessibleContentId;
      sharedStateKeys.add(sharedStateKey);
    }
    tracker.stop(
        "", "getAccessibleSharedStates", "accessibleSharedStatesSize", sharedStateKeys.size());
    return sharedStateKeys;
  }

  private Set<String> getLocalActionSemanticProperties(Set<StreamLocalAction> actions) {
    ElapsedTimeTracker tracker = timingUtils.getElapsedTimeTracker(TAG);
    Set<String> semanticPropertiesKeys = new HashSet<>();
    for (StreamLocalAction action : actions) {
      String semanticPropertyKey = SEMANTIC_PROPERTIES_PREFIX + action.getFeatureContentId();
      semanticPropertiesKeys.add(semanticPropertyKey);
    }
    tracker.stop(
        "",
        "getLocalActionSemanticProperties",
        "actionSemanticPropertiesSize",
        semanticPropertiesKeys.size());
    return semanticPropertiesKeys;
  }
}

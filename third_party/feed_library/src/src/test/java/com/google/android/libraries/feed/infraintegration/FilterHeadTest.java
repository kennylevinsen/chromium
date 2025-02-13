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

package com.google.android.libraries.feed.infraintegration;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.MockitoAnnotations.initMocks;

import com.google.android.libraries.feed.api.client.requestmanager.RequestManager;
import com.google.android.libraries.feed.api.internal.sessionmanager.FeedSessionManager;
import com.google.android.libraries.feed.common.functional.Consumer;
import com.google.android.libraries.feed.common.functional.Function;
import com.google.android.libraries.feed.common.testing.InfraIntegrationScope;
import com.google.android.libraries.feed.common.testing.ResponseBuilder;
import com.google.android.libraries.feed.testing.requestmanager.FakeFeedRequestManager;
import com.google.search.now.feed.client.StreamDataProto.StreamFeature;
import com.google.search.now.feed.client.StreamDataProto.StreamPayload;
import com.google.search.now.wire.feed.ContentIdProto.ContentId;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

/** Tests of the {@link FeedSessionManager#getStreamFeaturesFromHead(Function, Consumer)} method. */
@RunWith(RobolectricTestRunner.class)
public class FilterHeadTest {
  private FakeFeedRequestManager fakeFeedRequestManager;

  private FeedSessionManager feedSessionManager;
  private RequestManager requestManager;

  @Before
  public void setUp() {
    initMocks(this);
    InfraIntegrationScope scope = new InfraIntegrationScope.Builder().build();
    fakeFeedRequestManager = scope.getFakeFeedRequestManager();
    feedSessionManager = scope.getFeedSessionManager();
    requestManager = scope.getRequestManager();
  }

  @Test
  public void testFiltering() {
    ContentId[] cards =
        new ContentId[] {
          ResponseBuilder.createFeatureContentId(1),
          ResponseBuilder.createFeatureContentId(2),
          ResponseBuilder.createFeatureContentId(3),
          ResponseBuilder.createFeatureContentId(4),
          ResponseBuilder.createFeatureContentId(5),
        };

    fakeFeedRequestManager.queueResponse(ResponseBuilder.forClearAllWithCards(cards).build());
    requestManager.triggerScheduledRefresh();

    feedSessionManager.getStreamFeaturesFromHead(
        transformer,
        result -> {
          assertThat(result.isSuccessful()).isTrue();
          assertThat(result.getValue()).hasSize(11);
        });

    // only return the contentId of the Content cards
    feedSessionManager.getStreamFeaturesFromHead(
        toContent,
        result -> {
          assertThat(result.isSuccessful()).isTrue();
          assertThat(result.getValue()).hasSize(5);
        });
  }

  // Return only StreamFeatures
  private Function<StreamPayload, /*@Nullable*/ StreamFeature> transformer =
      streamPayload -> streamPayload.hasStreamFeature() ? streamPayload.getStreamFeature() : null;

  // Return the contentId from a feature with a RenderableUnit type of CONTENT
  private Function<StreamPayload, /*@Nullable*/ String> toContent =
      streamPayload -> {
        if (!streamPayload.hasStreamFeature()) {
          return null;
        }
        StreamFeature streamFeature = streamPayload.getStreamFeature();
        return streamFeature.hasContent() ? streamFeature.getContentId() : null;
      };
}

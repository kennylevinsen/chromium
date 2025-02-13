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

package com.google.android.libraries.feed.testing.conformance.network;

import static com.google.common.truth.Truth.assertThat;

import android.net.Uri;
import com.google.android.libraries.feed.api.host.network.HttpRequest;
import com.google.android.libraries.feed.api.host.network.HttpRequest.HttpMethod;
import com.google.android.libraries.feed.api.host.network.HttpResponse;
import com.google.android.libraries.feed.api.host.network.NetworkClient;
import com.google.android.libraries.feed.common.testing.RequiredConsumer;
import java.util.Collections;
import org.junit.Test;

public abstract class NetworkClientConformanceTest {

  protected NetworkClient networkClient;

  /** Defines a valid URI for the provided method. */
  protected Uri getValidUri(@HttpMethod String httpMethod) {
    return new Uri.Builder().path("http://www.google.com/").build();
  }

  /** Allows conformance tests to delay assertions until after the request completes. */
  protected void waitForRequest() {}

  @Test
  public void send_get() {
    @HttpMethod String method = HttpMethod.GET;
    HttpRequest request =
        new HttpRequest(getValidUri(method), method, Collections.emptyList(), new byte[] {});
    RequiredConsumer<HttpResponse> responseConsumer =
        new RequiredConsumer<>(
            response -> {
              assertThat(response.getResponseBody()).isNotNull();
              assertThat(response.getResponseCode()).isNotNull();
            });

    networkClient.send(request, responseConsumer);
    waitForRequest();
    assertThat(responseConsumer.isCalled()).isTrue();
  }

  @Test
  public void send_post() {
    @HttpMethod String method = HttpMethod.POST;
    HttpRequest request =
        new HttpRequest(
            getValidUri(method), method, Collections.emptyList(), "helloWorld".getBytes());
    RequiredConsumer<HttpResponse> responseConsumer =
        new RequiredConsumer<>(
            response -> {
              assertThat(response.getResponseBody()).isNotNull();
              assertThat(response.getResponseCode()).isNotNull();
            });

    networkClient.send(request, responseConsumer);
    waitForRequest();
    assertThat(responseConsumer.isCalled()).isTrue();
  }

  @Test
  public void send_put() {
    @HttpMethod String method = HttpMethod.PUT;
    HttpRequest request =
        new HttpRequest(
            getValidUri(method), method, Collections.emptyList(), "helloWorld".getBytes());
    RequiredConsumer<HttpResponse> responseConsumer =
        new RequiredConsumer<>(
            response -> {
              assertThat(response.getResponseBody()).isNotNull();
              assertThat(response.getResponseCode()).isNotNull();
            });

    networkClient.send(request, responseConsumer);
    waitForRequest();
    assertThat(responseConsumer.isCalled()).isTrue();
  }

  @Test
  public void send_delete() {
    @HttpMethod String method = HttpMethod.DELETE;
    HttpRequest request =
        new HttpRequest(getValidUri(method), method, Collections.emptyList(), new byte[] {});
    RequiredConsumer<HttpResponse> responseConsumer =
        new RequiredConsumer<>(
            response -> {
              assertThat(response.getResponseBody()).isNotNull();
              assertThat(response.getResponseCode()).isNotNull();
            });

    networkClient.send(request, responseConsumer);
    waitForRequest();
    assertThat(responseConsumer.isCalled()).isTrue();
  }
}

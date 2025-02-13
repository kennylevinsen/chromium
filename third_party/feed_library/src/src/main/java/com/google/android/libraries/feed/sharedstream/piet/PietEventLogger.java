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

package com.google.android.libraries.feed.sharedstream.piet;

import com.google.android.libraries.feed.api.host.logging.BasicLoggingApi;
import com.google.search.now.ui.piet.ErrorsProto.ErrorCode;
import java.util.ArrayList;
import java.util.List;

/** Logger that logs to the {@link BasicLoggingApi} any Piet errors. */
public class PietEventLogger {

  private final BasicLoggingApi basicLoggingApi;

  public PietEventLogger(BasicLoggingApi basicLoggingApi) {
    this.basicLoggingApi = basicLoggingApi;
  }

  public void logEvents(List<ErrorCode> pietErrors) {
    basicLoggingApi.onPietFrameRenderingEvent(convertErrorCodes(pietErrors));
  }

  private List<Integer> convertErrorCodes(List<ErrorCode> errorCodes) {
    List<Integer> errorCodeValues = new ArrayList<>(errorCodes.size());
    for (ErrorCode errorCode : errorCodes) {
      errorCodeValues.add(errorCode.getNumber());
    }
    return errorCodeValues;
  }
}

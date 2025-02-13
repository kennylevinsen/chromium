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

import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.piet.host.HostBindingProvider;
import com.google.android.libraries.feed.sharedstream.offlinemonitor.StreamOfflineMonitor;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.HostBindingData;
import com.google.search.now.ui.stream.StreamOfflineExtensionProto.OfflineExtension;

/**
 * A Stream implementation of a {@link HostBindingProvider} which handles Stream host bindings and
 * can delegate to a host host binding provider if needed.
 */
public class PietHostBindingProvider extends HostBindingProvider {

  private static final String TAG = "PietHostBindingProvider";

  private final StreamOfflineMonitor offlineMonitor;
  /*@Nullable*/ private final HostBindingProvider hostHostBindingProvider;

  public PietHostBindingProvider(
      /*@Nullable*/ HostBindingProvider hostHostBindingProvider, StreamOfflineMonitor offlineMonitor) {
    this.hostHostBindingProvider = hostHostBindingProvider;
    this.offlineMonitor = offlineMonitor;
  }

  @Override
  public BindingValue getCustomElementDataBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getCustomElementDataBindingForValue(bindingValue);
    }
    return super.getCustomElementDataBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getParameterizedTextBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getParameterizedTextBindingForValue(bindingValue);
    }
    return super.getParameterizedTextBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getChunkedTextBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getChunkedTextBindingForValue(bindingValue);
    }
    return super.getChunkedTextBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getImageBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getImageBindingForValue(bindingValue);
    }
    return super.getImageBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getActionsBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getActionsBindingForValue(bindingValue);
    }
    return super.getActionsBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getGridCellWidthBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getGridCellWidthBindingForValue(bindingValue);
    }
    return super.getGridCellWidthBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getLogDataBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getLogDataBindingForValue(bindingValue);
    }
    return super.getLogDataBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getTemplateBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getTemplateBindingForValue(bindingValue);
    }
    return super.getTemplateBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getStyleBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getStyleBindingForValue(bindingValue);
    }
    return super.getStyleBindingForValue(bindingValue);
  }

  @Override
  public BindingValue getVisibilityBindingForValue(BindingValue bindingValue) {
    BindingValue genericBindingResult = getGenericBindingForValue(bindingValue);

    if (genericBindingResult != null) {
      return genericBindingResult;
    }

    if (hostHostBindingProvider != null) {
      return hostHostBindingProvider.getVisibilityBindingForValue(bindingValue);
    }

    return super.getVisibilityBindingForValue(bindingValue);
  }

  /**
   * Gets a {@link BindingValue} that supports multiple separate types. IE, Visibility or Style
   * bindings. Returns {@literal null} if no generic binding can be found.
   */
  /*@Nullable*/
  private BindingValue getGenericBindingForValue(BindingValue bindingValue) {
    HostBindingData hostBindingData = bindingValue.getHostBindingData();

    if (hostBindingData.hasExtension(OfflineExtension.offlineExtension)) {
      BindingValue result =
          getBindingForOfflineExtension(
              hostBindingData.getExtension(OfflineExtension.offlineExtension));
      if (result != null) {
        return result;
      }
    }

    return null;
  }

  /*@Nullable*/
  private BindingValue getBindingForOfflineExtension(OfflineExtension offlineExtension) {
    if (!offlineExtension.hasUrl()) {
      Logger.e(TAG, "No URL for OfflineExtension, return clear.");
      return null;
    }

    return offlineMonitor.isAvailableOffline(offlineExtension.getUrl())
        ? offlineExtension.getOfflineBinding()
        : offlineExtension.getNotOfflineBinding();
  }
}

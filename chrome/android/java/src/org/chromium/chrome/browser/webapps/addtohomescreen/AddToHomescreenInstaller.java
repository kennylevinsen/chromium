// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps.addtohomescreen;

import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.PackageUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.banners.AppData;
import org.chromium.chrome.browser.tab.Tab;

/**
 * Provides functionality related to native Android apps for its C++ counterpart,
 * add_to_homescreen_installer.cc.
 */
class AddToHomescreenInstaller {
    private static final String TAG = "AddToHomescreen";

    @CalledByNative
    private static boolean installOrOpenNativeApp(Tab tab, AppData appData) {
        Context context = ContextUtils.getApplicationContext();
        Intent launchIntent;
        if (PackageUtils.isPackageInstalled(context, appData.packageName())) {
            launchIntent =
                    context.getPackageManager().getLaunchIntentForPackage(appData.packageName());
        } else {
            launchIntent = appData.installIntent();
        }
        if (launchIntent != null && tab.getActivity() != null) {
            try {
                tab.getActivity().startActivity(launchIntent);
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Failed to install or open app : %s!", appData.packageName(), e);
                return false;
            }
        }
        return true;
    }
}

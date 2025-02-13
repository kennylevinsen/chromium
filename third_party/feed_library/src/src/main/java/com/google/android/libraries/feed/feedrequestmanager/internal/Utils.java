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

package com.google.android.libraries.feed.feedrequestmanager.internal;

import android.text.TextUtils;
import com.google.android.libraries.feed.api.host.config.ApplicationInfo.AppType;
import com.google.android.libraries.feed.api.host.config.ApplicationInfo.Architecture;
import com.google.android.libraries.feed.api.host.config.ApplicationInfo.BuildType;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.search.now.wire.feed.ClientInfoProto.ClientInfo;
import com.google.search.now.wire.feed.VersionProto.Version;
import java.text.NumberFormat;
import java.text.ParseException;

/** Parsing and conversion static methods. */
public final class Utils {

  private static final String TAG = "Utils";

  // Do not instantiate
  private Utils() {}

  /**
   * Fills in the major/minor/build/revision part of a {@link Version} builder from a given string.
   * A version string typically looks like: 'major.minor.build.revision'.
   */
  public static void fillVersionsFromString(Version.Builder versionBuilder, String versionString) {
    String[] values = TextUtils.split(versionString, "\\.");
    if (values.length >= 1) {
      try {
        versionBuilder.setMajor(parseLeadingInt(values[0]));
        if (values.length >= 2) {
          versionBuilder.setMinor(parseLeadingInt(values[1]));
          if (values.length >= 3) {
            versionBuilder.setBuild(parseLeadingInt(values[2]));
            if (values.length >= 4) {
              versionBuilder.setRevision(parseLeadingInt(values[3]));
            }
          }
        }
      } catch (ParseException e) {
        Logger.w(TAG, e, "Invalid int value while parsing string version: %s", versionString);
      }
    } else {
      Logger.w(TAG, "Invalid format while parsing string version: %s" + versionString);
    }
  }

  public static ClientInfo.AppType convertAppType(@AppType int type) {
    switch (type) {
      case AppType.SEARCH_APP:
        return ClientInfo.AppType.GSA;
      case AppType.CHROME:
        return ClientInfo.AppType.CHROME;
      case AppType.TEST_APP:
        return ClientInfo.AppType.TEST_APP;
      default:
        return ClientInfo.AppType.UNKNOWN_APP;
    }
  }

  public static Version.Architecture convertArchitecture(@Architecture int architecture) {
    switch (architecture) {
      case Architecture.ARM:
        return Version.Architecture.ARM;
      case Architecture.ARM64:
        return Version.Architecture.ARM64;
      case Architecture.MIPS:
        return Version.Architecture.MIPS;
      case Architecture.MIPS64:
        return Version.Architecture.MIPS64;
      case Architecture.X86:
        return Version.Architecture.X86;
      case Architecture.X86_64:
        return Version.Architecture.X86_64;
      default:
        return Version.Architecture.UNKNOWN_ACHITECTURE;
    }
  }

  public static Version.Architecture convertArchitectureString(String architecture) {
    switch (architecture) {
      case "armeabi":
      case "armeabi-v7a":
        return Version.Architecture.ARM;
      case "arm64-v8a":
        return Version.Architecture.ARM64;
      case "mips":
        return Version.Architecture.MIPS;
      case "mips64":
        return Version.Architecture.MIPS64;
      case "x86":
        return Version.Architecture.X86;
      case "x86_64":
        return Version.Architecture.X86_64;
      default:
        return Version.Architecture.UNKNOWN_ACHITECTURE;
    }
  }

  public static Version.BuildType convertBuildType(@BuildType int buildType) {
    switch (buildType) {
      case BuildType.DEV:
        return Version.BuildType.DEV;
      case BuildType.ALPHA:
        return Version.BuildType.ALPHA;
      case BuildType.BETA:
        return Version.BuildType.BETA;
      case BuildType.RELEASE:
        return Version.BuildType.RELEASE;
      default:
        return Version.BuildType.UNKNOWN_BUILD_TYPE;
    }
  }

  private static int parseLeadingInt(String value) throws ParseException {
    NumberFormat integerParser = NumberFormat.getIntegerInstance();
    return integerParser.parse(value).intValue();
  }
}

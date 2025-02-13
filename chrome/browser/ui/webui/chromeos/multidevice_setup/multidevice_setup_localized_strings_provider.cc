// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/multidevice_setup/multidevice_setup_localized_strings_provider.h"

#include "base/no_destructor.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/system/sys_info.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/chromeos/multidevice_setup/multidevice_setup_handler.h"
#include "chrome/browser/ui/webui/localized_string.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/multidevice_setup_resources.h"
#include "chrome/grit/multidevice_setup_resources_map.h"
#include "chromeos/grit/chromeos_resources.h"
#include "chromeos/services/multidevice_setup/public/cpp/url_provider.h"
#include "chromeos/services/multidevice_setup/public/mojom/constants.mojom.h"
#include "components/login/localized_values_builder.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/chromeos/devicetype_utils.h"

namespace chromeos {

namespace multidevice_setup {

namespace {

const char kFootnoteMarker[] = "*";

constexpr LocalizedString kLocalizedStringsWithoutPlaceholders[] = {
    {"accept", IDS_MULTIDEVICE_SETUP_ACCEPT_LABEL},
    {"back", IDS_MULTIDEVICE_SETUP_BACK_LABEL},
    {"cancel", IDS_CANCEL},
    {"done", IDS_DONE},
    {"noThanks", IDS_NO_THANKS},
    {"passwordPageHeader", IDS_MULTIDEVICE_SETUP_PASSWORD_PAGE_HEADER},
    {"enterPassword", IDS_MULTIDEVICE_SETUP_PASSWORD_PAGE_ENTER_PASSWORD_LABEL},
    {"wrongPassword", IDS_MULTIDEVICE_SETUP_PASSWORD_PAGE_WRONG_PASSWORD_LABEL},
    {"startSetupPageMultipleDeviceHeader",
     IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_MULTIPLE_DEVICE_HEADER},
    {"startSetupPageSingleDeviceHeader",
     IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_SINGLE_DEVICE_HEADER},
    {"startSetupPageOfflineDeviceOption",
     IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_OFFLINE_DEVICE_OPTION},
    {"startSetupPageFeatureListInstallApps",
     IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_INSTALL_APPS_DESCRIPTION},
    {"startSetupPageFeatureListAddFeatures",
     IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_ADD_FEATURES},
    {"setupSucceededPageHeader",
     IDS_MULTIDEVICE_SETUP_SETUP_SUCCEEDED_PAGE_HEADER},
    {"setupSucceededPageMessage",
     IDS_MULTIDEVICE_SETUP_SETUP_SUCCEEDED_PAGE_MESSAGE},
    {"startSetupPageHeader", IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_HEADER},
    {"tryAgain", IDS_MULTIDEVICE_SETUP_TRY_AGAIN_LABEL},
    {"dialogAccessibilityTitle",
     IDS_MULTIDEVICE_SETUP_DIALOG_ACCESSIBILITY_TITLE},
};

struct LocalizedStringWithName {
  LocalizedStringWithName(const char* name,
                          const base::string16& localized_string)
      : name(name), localized_string(localized_string) {}

  const char* name;
  base::string16 localized_string;
};

const std::vector<LocalizedStringWithName>&
GetLocalizedStringsWithPlaceholders() {
  static const base::NoDestructor<std::vector<LocalizedStringWithName>>
      localized_strings([] {
        std::vector<LocalizedStringWithName> localized_strings;

        // TODO(crbug.com/964547): Refactor so that any change to these strings
        // will surface in both the OOBE and post-OOBE UIs without having to
        // adjust both localization calls separately.
        localized_strings.emplace_back(
            "startSetupPageMessage",
            l10n_util::GetStringFUTF16(
                IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_MESSAGE,
                ui::GetChromeOSDeviceName(),
                base::ASCIIToUTF16(kFootnoteMarker),
                base::UTF8ToUTF16(
                    chromeos::multidevice_setup::
                        GetBoardSpecificBetterTogetherSuiteLearnMoreUrl()
                            .spec())));

        localized_strings.emplace_back(
            "startSetupPageFootnote",
            l10n_util::GetStringFUTF16(
                IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_FOOTNOTE,
                base::ASCIIToUTF16(kFootnoteMarker)));

        localized_strings.emplace_back(
            "startSetupPageFeatureListHeader",
            l10n_util::GetStringFUTF16(
                IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_FEATURE_LIST_HEADER,
                ui::GetChromeOSDeviceName()));

        localized_strings.emplace_back(
            "startSetupPageFeatureListAwm",
            l10n_util::GetStringFUTF16(
                IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_AWM_DESCRIPTION,
                base::UTF8ToUTF16(chromeos::multidevice_setup::
                                      GetBoardSpecificMessagesLearnMoreUrl()
                                          .spec())));

        return localized_strings;
      }());

  return *localized_strings;
}

}  //  namespace

void AddLocalizedStrings(content::WebUIDataSource* html_source) {
  AddLocalizedStringsBulk(html_source, kLocalizedStringsWithoutPlaceholders,
                          base::size(kLocalizedStringsWithoutPlaceholders));

  for (const auto& entry : GetLocalizedStringsWithPlaceholders())
    html_source->AddString(entry.name, entry.localized_string);
}

void AddLocalizedValuesToBuilder(::login::LocalizedValuesBuilder* builder) {
  for (const auto& entry : kLocalizedStringsWithoutPlaceholders)
    builder->Add(entry.name, entry.id);

  // TODO(crbug.com/964547): Refactor so that any change to these strings will
  // surface in both the OOBE and post-OOBE UIs without having to adjust both
  // localization calls separately.
  builder->AddF(
      "startSetupPageMessage", IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_MESSAGE,
      ui::GetChromeOSDeviceName(), base::ASCIIToUTF16(kFootnoteMarker),
      base::UTF8ToUTF16(chromeos::multidevice_setup::
                            GetBoardSpecificBetterTogetherSuiteLearnMoreUrl()
                                .spec()));

  builder->AddF("startSetupPageFeatureListHeader",
                IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_FEATURE_LIST_HEADER,
                ui::GetChromeOSDeviceName());

  builder->AddF("startSetupPageFootnote",
                IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_FOOTNOTE,
                base::ASCIIToUTF16(kFootnoteMarker));

  builder->AddF(
      "startSetupPageFeatureListAwm",
      IDS_MULTIDEVICE_SETUP_START_SETUP_PAGE_AWM_DESCRIPTION,
      base::UTF8ToUTF16(
          chromeos::multidevice_setup::GetBoardSpecificMessagesLearnMoreUrl()
              .spec()));
}

}  // namespace multidevice_setup

}  // namespace chromeos

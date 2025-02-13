// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/welcome/welcome_ui.h"

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "build/branding_buildflags.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/ui/webui/localized_string.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/browser/ui/webui/welcome/bookmark_handler.h"
#include "chrome/browser/ui/webui/welcome/google_apps_handler.h"
#include "chrome/browser/ui/webui/welcome/helpers.h"
#include "chrome/browser/ui/webui/welcome/ntp_background_handler.h"
#include "chrome/browser/ui/webui/welcome/set_as_default_handler.h"
#include "chrome/browser/ui/webui/welcome/welcome_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chrome_unscaled_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/welcome_resources.h"
#include "chrome/grit/welcome_resources_map.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/strings/grit/components_strings.h"
#include "net/base/url_util.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace {

constexpr char kGeneratedPath[] =
    "@out_folder@/gen/chrome/browser/resources/welcome/";

const char kPreviewBackgroundPath[] = "preview-background.jpg";

bool ShouldHandleRequestCallback(base::WeakPtr<WelcomeUI> weak_ptr,
                                 const std::string& path) {
  if (!base::StartsWith(path, kPreviewBackgroundPath,
                        base::CompareCase::SENSITIVE)) {
    return false;
  }

  std::string index_param = path.substr(path.find_first_of("?") + 1);
  int background_index = -1;
  if (!base::StringToInt(index_param, &background_index) ||
      background_index < 0) {
    return false;
  }

  return !weak_ptr ? false : true;
}

void HandleRequestCallback(
    base::WeakPtr<WelcomeUI> weak_ptr,
    const std::string& path,
    const content::WebUIDataSource::GotDataCallback& callback) {
  DCHECK(ShouldHandleRequestCallback(weak_ptr, path));

  std::string index_param = path.substr(path.find_first_of("?") + 1);
  int background_index = -1;
  CHECK(base::StringToInt(index_param, &background_index) ||
        background_index < 0);

  DCHECK(weak_ptr);
  weak_ptr->CreateBackgroundFetcher(background_index, callback);
}

void AddStrings(content::WebUIDataSource* html_source) {
  static constexpr LocalizedString kLocalizedStrings[] = {
      // Shared strings.
      {"bookmarkAdded", IDS_WELCOME_BOOKMARK_ADDED},
      {"bookmarksAdded", IDS_WELCOME_BOOKMARKS_ADDED},
      {"bookmarkRemoved", IDS_WELCOME_BOOKMARK_REMOVED},
      {"bookmarksRemoved", IDS_WELCOME_BOOKMARKS_REMOVED},
      {"defaultBrowserChanged", IDS_DEFAULT_BROWSER_CHANGED},
      {"headerText", IDS_WELCOME_HEADER},
      {"next", IDS_WELCOME_NEXT},
      {"noThanks", IDS_NO_THANKS},
      {"skip", IDS_WELCOME_SKIP},

      // Sign-in view strings.
      {"signInHeader", IDS_WELCOME_SIGNIN_VIEW_HEADER},
      {"signInSubHeader", IDS_WELCOME_SIGNIN_VIEW_SUB_HEADER},
      {"signIn", IDS_WELCOME_SIGNIN_VIEW_SIGNIN},

      // Google apps module strings.
      {"googleAppsDescription", IDS_WELCOME_GOOGLE_APPS_DESCRIPTION},

      // New Tab Page background module strings.
      {"ntpBackgroundDescription", IDS_WELCOME_NTP_BACKGROUND_DESCRIPTION},
      {"ntpBackgroundDefault", IDS_WELCOME_NTP_BACKGROUND_DEFAULT_TITLE},
      {"ntpBackgroundPreviewUpdated",
       IDS_WELCOME_NTP_BACKGROUND_PREVIEW_UPDATED},
      {"ntpBackgroundReset", IDS_WELCOME_NTP_BACKGROUND_RESET},

      // Set as default module strings.
      {"setDefaultHeader", IDS_WELCOME_SET_AS_DEFAULT_HEADER},
      {"setDefaultSubHeader", IDS_WELCOME_SET_AS_DEFAULT_SUB_HEADER},
      {"setDefaultConfirm", IDS_WELCOME_SET_AS_DEFAULT_SET_AS_DEFAULT},

      // Landing view strings.
      {"landingTitle", IDS_WELCOME_LANDING_TITLE},
      {"landingDescription", IDS_WELCOME_LANDING_DESCRIPTION},
      {"landingNewUser", IDS_WELCOME_LANDING_NEW_USER},
      {"landingExistingUser", IDS_WELCOME_LANDING_EXISTING_USER},
  };
  AddLocalizedStringsBulk(html_source, kLocalizedStrings,
                          base::size(kLocalizedStrings));
}

}  // namespace

WelcomeUI::WelcomeUI(content::WebUI* web_ui, const GURL& url)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  // This page is not shown to incognito or guest profiles. If one should end up
  // here, we return, causing a 404-like page.
  if (!profile || !profile->IsRegularProfile()) {
    return;
  }

  StorePageSeen(profile);

  web_ui->AddMessageHandler(std::make_unique<WelcomeHandler>(web_ui));

  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(url.host());
  webui::SetupWebUIDataSource(
      html_source, base::make_span(kWelcomeResources, kWelcomeResourcesSize),
      kGeneratedPath, IDR_WELCOME_HTML);

  // Add welcome strings.
  AddStrings(html_source);

#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  // Load unscaled images.
  html_source->AddResourcePath("images/module_icons/google_dark.svg",
                               IDR_WELCOME_MODULE_ICONS_GOOGLE_DARK);
  html_source->AddResourcePath("images/module_icons/google_light.svg",
                               IDR_WELCOME_MODULE_ICONS_GOOGLE_LIGHT);
  html_source->AddResourcePath("images/module_icons/set_default_dark.svg",
                               IDR_WELCOME_MODULE_ICONS_SET_DEFAULT_DARK);
  html_source->AddResourcePath("images/module_icons/set_default_light.svg",
                               IDR_WELCOME_MODULE_ICONS_SET_DEFAULT_LIGHT);
  html_source->AddResourcePath("images/module_icons/wallpaper_dark.svg",
                               IDR_WELCOME_MODULE_ICONS_WALLPAPER_DARK);
  html_source->AddResourcePath("images/module_icons/wallpaper_light.svg",
                               IDR_WELCOME_MODULE_ICONS_WALLPAPER_LIGHT);
  html_source->AddResourcePath("images/ntp_thumbnails/art.jpg",
                               IDR_WELCOME_NTP_THUMBNAILS_ART);
  html_source->AddResourcePath("images/ntp_thumbnails/cityscape.jpg",
                               IDR_WELCOME_NTP_THUMBNAILS_CITYSCAPE);
  html_source->AddResourcePath("images/ntp_thumbnails/earth.jpg",
                               IDR_WELCOME_NTP_THUMBNAILS_EARTH);
  html_source->AddResourcePath("images/ntp_thumbnails/geometric_shapes.jpg",
                               IDR_WELCOME_NTP_THUMBNAILS_GEOMETRIC_SHAPES);
  html_source->AddResourcePath("images/ntp_thumbnails/landscape.jpg",
                               IDR_WELCOME_NTP_THUMBNAILS_LANDSCAPE);
  html_source->AddResourcePath("images/set_default_dark.svg",
                               IDR_WELCOME_SET_DEFAULT_DARK);
  html_source->AddResourcePath("images/set_default_light.svg",
                               IDR_WELCOME_SET_DEFAULT_LIGHT);
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)

#if defined(OS_WIN)
  html_source->AddBoolean("is_win10",
                          base::win::GetVersion() >= base::win::Version::WIN10);
#endif

  // Add the shared bookmark handler for welcome modules.
  web_ui->AddMessageHandler(
      std::make_unique<welcome::BookmarkHandler>(profile->GetPrefs()));

  // Add google apps bookmarking module.
  web_ui->AddMessageHandler(std::make_unique<welcome::GoogleAppsHandler>());

  // Add NTP custom background module.
  web_ui->AddMessageHandler(std::make_unique<welcome::NtpBackgroundHandler>());

  // Add set-as-default module.
  web_ui->AddMessageHandler(std::make_unique<welcome::SetAsDefaultHandler>());

  html_source->AddString(
      "newUserModules",
      welcome::GetModules(profile).FindKey("new-user")->GetString());
  html_source->AddString(
      "returningUserModules",
      welcome::GetModules(profile).FindKey("returning-user")->GetString());
  html_source->AddBoolean(
      "signinAllowed", profile->GetPrefs()->GetBoolean(prefs::kSigninAllowed));
  html_source->SetRequestFilter(
      base::BindRepeating(&ShouldHandleRequestCallback,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&HandleRequestCallback,
                          weak_ptr_factory_.GetWeakPtr()));

  content::WebUIDataSource::Add(profile, html_source);
}

WelcomeUI::~WelcomeUI() {}

void WelcomeUI::CreateBackgroundFetcher(
    size_t background_index,
    const content::WebUIDataSource::GotDataCallback& callback) {
  background_fetcher_ = std::make_unique<welcome::NtpBackgroundFetcher>(
      background_index, callback);
}

void WelcomeUI::StorePageSeen(Profile* profile) {
  // Store that this profile has been shown the Welcome page.
  profile->GetPrefs()->SetBoolean(prefs::kHasSeenWelcomePage, true);
}

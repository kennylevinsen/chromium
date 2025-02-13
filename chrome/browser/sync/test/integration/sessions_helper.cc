// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/sessions_helper.h"

#include <stddef.h>

#include <set>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/session_sync_service_factory.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_datatype_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/test/fake_server/fake_server.h"
#include "components/sync/test/fake_server/sessions_hierarchy.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "components/sync_sessions/session_sync_service.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

using sync_datatype_helper::test;

namespace sessions_helper {

namespace {

bool SessionsSyncBridgeHasTabWithURL(int browser_index, const GURL& url) {
  content::RunAllPendingInMessageLoop();
  const sync_sessions::SyncedSession* local_session;
  if (!GetLocalSession(browser_index, &local_session)) {
    return false;
  }

  if (local_session->windows.empty()) {
    DVLOG(1) << "Empty windows vector";
    return false;
  }

  int nav_index;
  sessions::SerializedNavigationEntry nav;
  for (auto it = local_session->windows.begin();
       it != local_session->windows.end(); ++it) {
    if (it->second->wrapped_window.tabs.empty()) {
      DVLOG(1) << "Empty tabs vector";
      continue;
    }
    for (auto tab_it = it->second->wrapped_window.tabs.begin();
         tab_it != it->second->wrapped_window.tabs.end(); ++tab_it) {
      if ((*tab_it)->navigations.empty()) {
        DVLOG(1) << "Empty navigations vector";
        continue;
      }
      nav_index = (*tab_it)->current_navigation_index;
      nav = (*tab_it)->navigations[nav_index];
      if (nav.virtual_url() == url) {
        DVLOG(1) << "Found tab with url " << url.spec();
        DVLOG(1) << "Timestamp is " << nav.timestamp().ToInternalValue();
        if (nav.title().empty()) {
          DVLOG(1) << "Title empty -- tab hasn't finished loading yet";
          continue;
        }
        return true;
      }
    }
  }
  DVLOG(1) << "Could not find tab with url " << url.spec();
  return false;
}

}  // namespace

bool GetLocalSession(int browser_index,
                     const sync_sessions::SyncedSession** session) {
  return SessionSyncServiceFactory::GetInstance()
      ->GetForProfile(test()->GetProfile(browser_index))
      ->GetOpenTabsUIDelegate()
      ->GetLocalSession(session);
}

bool OpenTab(int browser_index, const GURL& url) {
  DVLOG(1) << "Opening tab: " << url.spec() << " using browser "
           << browser_index << ".";
  TabStripModel* tab_strip =
      test()->GetBrowser(browser_index)->tab_strip_model();
  int tab_index = tab_strip->count();
  return OpenTabAtIndex(browser_index, tab_index, url);
}

bool OpenTabAtIndex(int browser_index, int tab_index, const GURL& url) {
  chrome::AddTabAt(test()->GetBrowser(browser_index), url, tab_index, true);
  return WaitForTabToLoad(browser_index, url,
                          test()
                              ->GetBrowser(browser_index)
                              ->tab_strip_model()
                              ->GetWebContentsAt(tab_index));
}

bool OpenMultipleTabs(int browser_index, const std::vector<GURL>& urls) {
  Browser* browser = test()->GetBrowser(browser_index);
  for (auto it = urls.begin(); it != urls.end(); ++it) {
    DVLOG(1) << "Opening tab: " << it->spec() << " using browser "
             << browser_index << ".";
    ShowSingletonTab(browser, *it);
  }
  return WaitForTabsToLoad(browser_index, urls);
}

bool OpenTabFromSourceIndex(int browser_index,
                            int index_of_source_tab,
                            const GURL& url,
                            WindowOpenDisposition disposition) {
  content::WebContents* source_contents =
      test()
          ->GetBrowser(browser_index)
          ->tab_strip_model()
          ->GetWebContentsAt(index_of_source_tab);

  content::OpenURLParams open_url_params(url, content::Referrer(), disposition,
                                         ui::PAGE_TRANSITION_LINK, false,
                                         false);
  open_url_params.source_render_frame_id =
      source_contents->GetMainFrame()->GetRoutingID();
  open_url_params.source_render_process_id =
      source_contents->GetMainFrame()->GetProcess()->GetID();

  content::WebContents* new_contents =
      source_contents->OpenURL(open_url_params);
  if (!new_contents) {
    return false;
  }

  return WaitForTabToLoad(browser_index, url, new_contents);
}

void CloseTab(int browser_index, int tab_index) {
  TabStripModel* tab_strip =
      test()->GetBrowser(browser_index)->tab_strip_model();
  tab_strip->CloseWebContentsAt(tab_index, TabStripModel::CLOSE_USER_GESTURE);
}

void MoveTab(int from_browser_index, int to_browser_index, int tab_index) {
  std::unique_ptr<content::WebContents> detached_contents =
      test()
          ->GetBrowser(from_browser_index)
          ->tab_strip_model()
          ->DetachWebContentsAt(tab_index);

  TabStripModel* target_strip =
      test()->GetBrowser(to_browser_index)->tab_strip_model();
  target_strip->InsertWebContentsAt(target_strip->count(),
                                    std::move(detached_contents),
                                    TabStripModel::ADD_ACTIVE);
}

void NavigateTab(int browser_index, const GURL& url) {
  NavigateParams params(test()->GetBrowser(browser_index), url,
                        ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
  ui_test_utils::NavigateToURL(&params);
}

void NavigateTabBack(int browser_index) {
  content::WebContents* web_contents =
      test()->GetBrowser(browser_index)->tab_strip_model()->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents);
  web_contents->GetController().GoBack();
  observer.WaitForNavigationFinished();
}

void NavigateTabForward(int browser_index) {
  content::WebContents* web_contents =
      test()->GetBrowser(browser_index)->tab_strip_model()->GetWebContentsAt(0);
  content::TestNavigationObserver observer(web_contents);
  web_contents->GetController().GoForward();
  observer.WaitForNavigationFinished();
}

bool ExecJs(int browser_index, int tab_index, const std::string& script) {
  return content::ExecJs(
      test()->GetBrowser(browser_index)->tab_strip_model()->GetWebContentsAt(0),
      script);
}

bool WaitForTabsToLoad(int browser_index, const std::vector<GURL>& urls) {
  int tab_index = 0;
  for (const auto& url : urls) {
    content::WebContents* web_contents = test()
                                             ->GetBrowser(browser_index)
                                             ->tab_strip_model()
                                             ->GetWebContentsAt(tab_index);
    if (!web_contents) {
      LOG(ERROR) << "Tab " << tab_index << " does not exist";
      return false;
    }
    bool success = WaitForTabToLoad(browser_index, url, web_contents);
    if (!success) {
      return false;
    }
    tab_index++;
  }
  return true;
}

bool WaitForTabToLoad(int browser_index,
                      const GURL& url,
                      content::WebContents* web_contents) {
  DCHECK(web_contents);
  DVLOG(1) << "Waiting for session to propagate to associator.";
  base::TimeTicks start_time = base::TimeTicks::Now();
  base::TimeTicks end_time = start_time + TestTimeouts::action_max_timeout();
  bool found = false;
  while (!found) {
    found = SessionsSyncBridgeHasTabWithURL(browser_index, url);
    if (base::TimeTicks::Now() >= end_time) {
      LOG(ERROR) << "Failed to find url " << url.spec() << " in tab after "
                 << TestTimeouts::action_max_timeout().InSecondsF()
                 << " seconds.";
      return false;
    }
    if (!found) {
      content::WaitForLoadStop(web_contents);
    }
  }
  return true;
}

bool GetLocalWindows(int browser_index, ScopedWindowMap* local_windows) {
  // The local session provided by GetLocalSession is owned, and has lifetime
  // controlled, by the sessions sync manager, so we must make our own copy.
  const sync_sessions::SyncedSession* local_session;
  if (!GetLocalSession(browser_index, &local_session)) {
    return false;
  }
  for (auto w = local_session->windows.begin();
       w != local_session->windows.end(); ++w) {
    const sessions::SessionWindow& window = w->second->wrapped_window;
    std::unique_ptr<sync_sessions::SyncedSessionWindow> new_window =
        std::make_unique<sync_sessions::SyncedSessionWindow>();
    new_window->wrapped_window.window_id =
        SessionID::FromSerializedValue(window.window_id.id());
    for (size_t t = 0; t < window.tabs.size(); ++t) {
      const sessions::SessionTab& tab = *window.tabs.at(t);
      std::unique_ptr<sessions::SessionTab> new_tab =
          std::make_unique<sessions::SessionTab>();
      new_tab->navigations.resize(tab.navigations.size());
      std::copy(tab.navigations.begin(), tab.navigations.end(),
                new_tab->navigations.begin());
      new_window->wrapped_window.tabs.push_back(std::move(new_tab));
    }
    auto id = new_window->wrapped_window.window_id;
    (*local_windows)[id] = std::move(new_window);
  }

  return true;
}

bool CheckInitialState(int browser_index) {
  if (0 != GetNumWindows(browser_index))
    return false;
  if (0 != GetNumForeignSessions(browser_index))
    return false;
  return true;
}

int GetNumWindows(int browser_index) {
  const sync_sessions::SyncedSession* local_session;
  if (!GetLocalSession(browser_index, &local_session)) {
    return 0;
  }
  return local_session->windows.size();
}

int GetNumForeignSessions(int browser_index) {
  SyncedSessionVector sessions;
  if (!SessionSyncServiceFactory::GetInstance()
           ->GetForProfile(test()->GetProfile(browser_index))
           ->GetOpenTabsUIDelegate()
           ->GetAllForeignSessions(&sessions)) {
    return 0;
  }
  return sessions.size();
}

bool GetSessionData(int browser_index, SyncedSessionVector* sessions) {
  if (!SessionSyncServiceFactory::GetInstance()
           ->GetForProfile(test()->GetProfile(browser_index))
           ->GetOpenTabsUIDelegate()
           ->GetAllForeignSessions(sessions)) {
    return false;
  }
  SortSyncedSessions(sessions);
  return true;
}

bool CompareSyncedSessions(const sync_sessions::SyncedSession* lhs,
                           const sync_sessions::SyncedSession* rhs) {
  if (!lhs || !rhs || lhs->windows.empty() || rhs->windows.empty()) {
    // Catchall for uncomparable data.
    return false;
  }

  return lhs->windows < rhs->windows;
}

void SortSyncedSessions(SyncedSessionVector* sessions) {
  std::sort(sessions->begin(), sessions->end(),
            CompareSyncedSessions);
}

bool NavigationEquals(const sessions::SerializedNavigationEntry& expected,
                      const sessions::SerializedNavigationEntry& actual) {
  if (expected.virtual_url() != actual.virtual_url()) {
    LOG(ERROR) << "Expected url " << expected.virtual_url()
               << ", actual " << actual.virtual_url();
    return false;
  }
  if (expected.referrer_url() != actual.referrer_url()) {
    LOG(ERROR) << "Expected referrer "
               << expected.referrer_url()
               << ", actual "
               << actual.referrer_url();
    return false;
  }
  if (expected.title() != actual.title()) {
    LOG(ERROR) << "Expected title " << expected.title()
               << ", actual " << actual.title();
    return false;
  }
  if (!ui::PageTransitionTypeIncludingQualifiersIs(expected.transition_type(),
                                                   actual.transition_type())) {
    LOG(ERROR) << "Expected transition "
               << expected.transition_type()
               << ", actual "
               << actual.transition_type();
    return false;
  }
  return true;
}

namespace {

template <typename T1, typename T2>
bool WindowsMatchImpl(const T1& win1, const T2& win2) {
  sessions::SessionTab* client0_tab;
  sessions::SessionTab* client1_tab;
  if (win1.size() != win2.size()) {
    LOG(ERROR) << "Win size doesn't match, win1 size: "
        << win1.size()
        << ", win2 size: "
        << win2.size();
    return false;
  }
  for (auto i = win1.begin(); i != win1.end(); ++i) {
    auto j = win2.find(i->first);
    if (j == win2.end()) {
      LOG(ERROR) << "Session doesn't match";
      return false;
    }
    if (i->second->wrapped_window.tabs.size() !=
        j->second->wrapped_window.tabs.size()) {
      LOG(ERROR) << "Tab size doesn't match, tab1 size: "
                 << i->second->wrapped_window.tabs.size()
                 << ", tab2 size: " << j->second->wrapped_window.tabs.size();
      return false;
    }
    for (size_t t = 0; t < i->second->wrapped_window.tabs.size(); ++t) {
      client0_tab = i->second->wrapped_window.tabs[t].get();
      client1_tab = j->second->wrapped_window.tabs[t].get();
      if (client0_tab->navigations.size() != client1_tab->navigations.size()) {
        return false;
      }
      for (size_t n = 0; n < client0_tab->navigations.size(); ++n) {
        if (!NavigationEquals(client0_tab->navigations[n],
                              client1_tab->navigations[n])) {
          return false;
        }
      }
    }
  }

  return true;
}

}  // namespace

bool WindowsMatch(const ScopedWindowMap& win1, const ScopedWindowMap& win2) {
  return WindowsMatchImpl(win1, win2);
}

bool WindowsMatch(const SessionWindowMap& win1, const ScopedWindowMap& win2) {
  return WindowsMatchImpl(win1, win2);
}

bool CheckForeignSessionsAgainst(int browser_index,
                                 const std::vector<ScopedWindowMap>& windows) {
  SyncedSessionVector sessions;

  if (!GetSessionData(browser_index, &sessions)) {
    LOG(ERROR) << "Cannot get session data";
    return false;
  }

  for (size_t w_index = 0; w_index < windows.size(); ++w_index) {
    // Skip the client's local window
    if (static_cast<int>(w_index) == browser_index) {
      continue;
    }

    size_t s_index = 0;

    for (; s_index < sessions.size(); ++s_index) {
      if (WindowsMatch(sessions[s_index]->windows, windows[w_index]))
        break;
    }

    if (s_index == sessions.size()) {
      LOG(ERROR) << "Cannot find window #" << w_index;
      return false;
    }
  }

  return true;
}

void DeleteForeignSession(int browser_index, std::string session_tag) {
  SessionSyncServiceFactory::GetInstance()
      ->GetForProfile(test()->GetProfile(browser_index))
      ->GetOpenTabsUIDelegate()
      ->DeleteForeignSession(session_tag);
}

}  // namespace sessions_helper

ForeignSessionsMatchChecker::ForeignSessionsMatchChecker(
    int browser_index,
    const std::vector<sessions_helper::ScopedWindowMap>& windows)
    : MultiClientStatusChangeChecker(
          sync_datatype_helper::test()->GetSyncServices()),
      browser_index_(browser_index),
      windows_(windows) {}

bool ForeignSessionsMatchChecker::IsExitConditionSatisfied(std::ostream* os) {
  *os << "Waiting for matching foreign sessions";
  return sessions_helper::CheckForeignSessionsAgainst(browser_index_, windows_);
}

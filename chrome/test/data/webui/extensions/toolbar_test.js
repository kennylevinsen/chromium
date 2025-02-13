// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {getInstance} from 'chrome://extensions/extensions.js';
import {assert} from 'chrome://resources/js/assert.m.js';
import {isChromeOS, isMac} from 'chrome://resources/js/cr.m.js';
import {flush} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {eventToPromise} from '../test_util.m.js';

import {TestService} from './test_service.js';
import {testVisible} from './test_util.js';

/** @fileoverview Suite of tests for extension-toolbar. */
window.extension_toolbar_tests = {};
extension_toolbar_tests.suiteName = 'ExtensionToolbarTest';
/** @enum {string} */
extension_toolbar_tests.TestNames = {
  Layout: 'layout',
  ClickHandlers: 'click handlers',
  DevModeToggle: 'dev mode toggle',
  KioskMode: 'kiosk mode button'
};

suite(extension_toolbar_tests.suiteName, function() {
  /** @type {MockDelegate} */
  let mockDelegate;

  /** @type {ExtensionsToolbarElement} */
  let toolbar;

  setup(function() {
    PolymerTest.clearBody();
    toolbar = document.createElement('extensions-toolbar');
    document.body.appendChild(toolbar);
    toolbar.inDevMode = false;
    toolbar.devModeControlledByPolicy = false;
    toolbar.isSupervised = false;
    if (isChromeOS) {
      toolbar.kioskEnabled = false;
    }
    mockDelegate = new TestService();
    toolbar.set('delegate', mockDelegate);

    // The toast manager is normally a child of the <extensions-manager>
    // element, so add it separately for this test.
    const toastManager = document.createElement('cr-toast-manager');
    document.body.appendChild(toastManager);
  });

  test(assert(extension_toolbar_tests.TestNames.Layout), function() {
    const boundTestVisible = testVisible.bind(null, toolbar);
    boundTestVisible('#devMode', true);
    assertEquals(toolbar.$.devMode.disabled, false);
    boundTestVisible('#loadUnpacked', false);
    boundTestVisible('#packExtensions', false);
    boundTestVisible('#updateNow', false);
    toolbar.set('inDevMode', true);
    flush();

    boundTestVisible('#devMode', true);
    assertEquals(toolbar.$.devMode.disabled, false);
    boundTestVisible('#loadUnpacked', true);
    boundTestVisible('#packExtensions', true);
    boundTestVisible('#updateNow', true);

    toolbar.set('canLoadUnpacked', false);
    flush();

    boundTestVisible('#devMode', true);
    boundTestVisible('#loadUnpacked', false);
    boundTestVisible('#packExtensions', true);
    boundTestVisible('#updateNow', true);
  });

  test(assert(extension_toolbar_tests.TestNames.DevModeToggle), function() {
    const toggle = toolbar.$.devMode;
    assertFalse(toggle.disabled);

    // Test that the dev-mode toggle is disabled when a policy exists.
    toolbar.set('devModeControlledByPolicy', true);
    flush();
    assertTrue(toggle.disabled);

    toolbar.set('devModeControlledByPolicy', false);
    flush();
    assertFalse(toggle.disabled);

    // Test that the dev-mode toggle is disabled when the user is supervised.
    toolbar.set('isSupervised', true);
    flush();
    assertTrue(toggle.disabled);
  });

  test(assert(extension_toolbar_tests.TestNames.ClickHandlers), function() {
    toolbar.set('inDevMode', true);
    flush();

    toolbar.$.devMode.click();
    return mockDelegate.whenCalled('setProfileInDevMode')
        .then(function(arg) {
          assertFalse(arg);
          mockDelegate.reset();
          toolbar.$.devMode.click();
          return mockDelegate.whenCalled('setProfileInDevMode');
        })
        .then(function(arg) {
          assertTrue(arg);
          toolbar.$.loadUnpacked.click();
          return mockDelegate.whenCalled('loadUnpacked');
        })
        .then(function() {
          const toastManager = getInstance();
          assertFalse(toastManager.isToastOpen);
          toolbar.$.updateNow.click();
          // Simulate user rapidly clicking update button multiple times.
          toolbar.$.updateNow.click();
          assertTrue(toastManager.isToastOpen);
          return mockDelegate.whenCalled('updateAllExtensions');
        })
        .then(function() {
          assertEquals(1, mockDelegate.getCallCount('updateAllExtensions'));
          assertFalse(!!toolbar.$$('extensions-pack-dialog'));
          toolbar.$.packExtensions.click();
          flush();
          const dialog = toolbar.$$('extensions-pack-dialog');
          assertTrue(!!dialog);

          if (!isMac) {
            const whenFocused =
                eventToPromise('focus', toolbar.$.packExtensions);
            dialog.$.dialog.cancel();
            return whenFocused;
          }
        });
  });

  if (isChromeOS) {
    test(assert(extension_toolbar_tests.TestNames.KioskMode), function() {
      const button = toolbar.$.kioskExtensions;
      expectTrue(button.hidden);
      toolbar.kioskEnabled = true;
      expectFalse(button.hidden);

      const whenTapped = eventToPromise('kiosk-tap', toolbar);
      button.click();
      return whenTapped;
    });
  }
});

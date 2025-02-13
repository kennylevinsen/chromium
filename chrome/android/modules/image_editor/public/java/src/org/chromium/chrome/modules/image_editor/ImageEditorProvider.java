// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.modules.image_editor;

import android.app.Activity;

import org.chromium.components.module_installer.builder.ModuleInterface;

/**
 * Interface to get access to the image editor activity.
 */
@ModuleInterface(module = "image_editor",
        impl = "org.chromium.chrome.modules.image_editor.ImageEditorProviderImpl")
public interface ImageEditorProvider {
    /**
     * Creates and returns the instance tied to the image editor activity.
     */
    ImageEditorCoordinator getImageEditorCoordinator(Activity activity);
}

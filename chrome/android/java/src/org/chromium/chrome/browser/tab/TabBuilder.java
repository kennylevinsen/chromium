// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab;

import org.chromium.chrome.browser.tab.TabUma.TabCreationState;
import org.chromium.chrome.browser.tabmodel.TabLaunchType;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;

/**
 * Builds {@link Tab} using builder pattern. All Tab classes should be instantiated
 * through this builder.
 */
public class TabBuilder {
    private int mId = Tab.INVALID_TAB_ID;
    private Tab mParent;
    private boolean mIncognito;
    private WindowAndroid mWindow;
    private Integer mLaunchType;
    private Integer mCreationType;
    private boolean mFromFrozenState;
    private LoadUrlParams mLoadUrlParams;

    private WebContents mWebContents;
    private TabDelegateFactory mDelegateFactory;
    private boolean mInitiallyHidden;
    private TabState mTabState;
    private boolean mUnfreeze;

    /**
     * Sets the id with which the Tab to create should be identified.
     * @param id The id of the Tab.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setId(int id) {
        mId = id;
        return this;
    }

    /**
     * Sets the tab from which the new one is opened.
     * @param parent The parent Tab.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setParent(Tab parent) {
        mParent = parent;
        return this;
    }

    /**
     * Sets incognito mode.
     * @param incognito {@code true} if the tab will be in incognito mode.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setIncognito(boolean incognito) {
        mIncognito = incognito;
        return this;
    }

    /**
     * Sets window which the Tab will be attached to.
     * @param window An instance of a {@link WindowAndroid}.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setWindow(WindowAndroid window) {
        mWindow = window;
        return this;
    }

    /**
     * Sets a flag indicating how this tab is launched (from a link, external app, etc).
     * @param type Launch type.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setLaunchType(@TabLaunchType int type) {
        mLaunchType = type;
        return this;
    }

    /**
     * Sets a {@link WebContents} object to be used on the Tab. If not set, a new one
     * will be created.
     * @param webContents {@link WebContents} object.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setWebContents(WebContents webContents) {
        mWebContents = webContents;
        return this;
    }

    /**
     * Sets a {@link TabDelegateFactory} object.
     * @param delegateFactory The factory delegated to create various Tab-related objects.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setDelegateFactory(TabDelegateFactory delegateFactory) {
        mDelegateFactory = delegateFactory;
        return this;
    }

    /**
     * Sets a flag indicating whether the Tab should start as hidden. Only used if
     * {@code webContents} is {@code null}.
     * @param initiallyHidden {@code true} if the newly created {@link WebContents} will be hidden.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setInitiallyHidden(boolean initiallyHidden) {
        mInitiallyHidden = initiallyHidden;
        return this;
    }

    /**
     * Sets a {@link TabState} object containing information about this Tab, if it was persisted.
     * @param tabState State object.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setTabState(TabState tabState) {
        mTabState = tabState;
        return this;
    }

    /**
     * Sets a flag indicating if there should be an attempt to restore state at the end of
     *        the initialization.
     * @param unfreeze {@code true} if WebContents needs restoring from its saved state.
     * @return {@link TabBuilder} creating the Tab.
     */
    public TabBuilder setUnfreeze(boolean unfreeze) {
        mUnfreeze = unfreeze;
        return this;
    }

    public Tab build() {
        // Pre-condition check
        if (mCreationType != null) {
            if (!mFromFrozenState) {
                assert mCreationType != TabCreationState.FROZEN_ON_RESTORE;
            } else {
                assert mLaunchType == TabLaunchType.FROM_RESTORE
                        && mCreationType == TabCreationState.FROZEN_ON_RESTORE;
            }
        } else {
            if (mFromFrozenState) assert mLaunchType == TabLaunchType.FROM_RESTORE;
        }

        Tab tab = new Tab(mId, mParent, mIncognito, mLaunchType);
        tab.updateWindowAndroid(mWindow);

        if (mParent != null && mDelegateFactory == null) {
            mDelegateFactory = mParent.getDelegateFactory();
        }

        // Initializes Tab. Its user data objects are also initialized through the event
        // |onInitialized| of TabObserver they register.
        tab.initialize(mParent, mCreationType, mLoadUrlParams, mWebContents, mDelegateFactory,
                mInitiallyHidden, mTabState, mUnfreeze);
        return tab;
    }

    private TabBuilder setCreationType(@TabCreationState int type) {
        mCreationType = type;
        return this;
    }

    private TabBuilder setFromFrozenState(boolean frozenState) {
        mFromFrozenState = frozenState;
        return this;
    }

    private TabBuilder setLoadUrlParams(LoadUrlParams loadUrlParams) {
        mLoadUrlParams = loadUrlParams;
        return this;
    }

    /**
     * Creates a TabBuilder for a new, "frozen" tab from a saved state. This can be used for
     * background tabs restored on cold start that should be loaded when switched to. initialize()
     * needs to be called afterwards to complete the second level initialization.
     */
    public static TabBuilder createFromFrozenState() {
        return new TabBuilder()
                .setLaunchType(TabLaunchType.FROM_RESTORE)
                .setCreationType(TabCreationState.FROZEN_ON_RESTORE)
                .setFromFrozenState(true);
    }

    /**
     * Creates a TabBuilder for a new tab to be loaded lazily. This can be used for tabs opened
     * in the background that should be loaded when switched to. initialize() needs to be called
     * afterwards to complete the second level initialization.
     * @param loadUrlParams Params specifying the conditions for loading url.
     */
    public static TabBuilder createForLazyLoad(LoadUrlParams loadUrlParams) {
        return new TabBuilder()
                .setLoadUrlParams(loadUrlParams)
                .setCreationType(TabCreationState.FROZEN_FOR_LAZY_LOAD);
    }

    /**
     * Creates a TabBuilder for a fresh tab. initialize() needs to be called afterwards to
     * complete the second level initialization.
     * @param initiallyHidden true iff the tab being created is initially in background
     */
    public static TabBuilder createLiveTab(boolean initiallyHidden) {
        return new TabBuilder().setCreationType(initiallyHidden
                        ? TabCreationState.LIVE_IN_BACKGROUND
                        : TabCreationState.LIVE_IN_FOREGROUND);
    }
}

// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_XDG_POPUP_WRAPPER_STABLE_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_XDG_POPUP_WRAPPER_STABLE_H_

#include <memory>

#include "ui/ozone/platform/wayland/host/xdg_popup_wrapper.h"

namespace ui {

class XDGSurfaceWrapper;
class WaylandConnection;
class WaylandWindow;

class XDGPopupWrapperStable : public XDGPopupWrapper {
 public:
  XDGPopupWrapperStable(std::unique_ptr<XDGSurfaceWrapper> surface,
                        WaylandWindow* wayland_window);
  ~XDGPopupWrapperStable() override;

  // XDGPopupWrapper:
  bool Initialize(WaylandConnection* connection,
                  wl_surface* surface,
                  WaylandWindow* parent_window,
                  const gfx::Rect& bounds) override;

  struct xdg_positioner* CreatePositioner(WaylandConnection* connection,
                                          WaylandWindow* parent_window,
                                          const gfx::Rect& bounds);

  // xdg_popup_listener
  static void Configure(void* data,
                        struct xdg_popup* xdg_popup,
                        int32_t x,
                        int32_t y,
                        int32_t width,
                        int32_t height);
  static void PopupDone(void* data, struct xdg_popup* xdg_popup);

  XDGSurfaceWrapper* xdg_surface();

 private:
  WaylandWindow* const wayland_window_;
  std::unique_ptr<XDGSurfaceWrapper> xdg_surface_;
  wl::Object<xdg_popup> xdg_popup_;

  DISALLOW_COPY_AND_ASSIGN(XDGPopupWrapperStable);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_XDG_POPUP_WRAPPER_STABLE_H_

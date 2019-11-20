// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_XDG_SURFACE_WRAPPER_STABLE_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_XDG_SURFACE_WRAPPER_STABLE_H_

#include "ui/ozone/platform/wayland/host/xdg_surface_wrapper.h"

#include "base/macros.h"

namespace gfx {
class Rect;
}

namespace ui {

class WaylandConnection;
class WaylandWindow;

class XDGSurfaceWrapperStable : public XDGSurfaceWrapper {
 public:
  XDGSurfaceWrapperStable(WaylandWindow* wayland_window);
  ~XDGSurfaceWrapperStable() override;

  // XDGSurfaceWrapper overrides:
  bool Initialize(WaylandConnection* connection,
                  wl_surface* surface,
                  bool with_toplevel) override;
  void SetMaximized() override;
  void UnSetMaximized() override;
  void SetFullscreen() override;
  void UnSetFullscreen() override;
  void SetMinimized() override;
  void SurfaceMove(WaylandConnection* connection) override;
  void SurfaceResize(WaylandConnection* connection, uint32_t hittest) override;
  void SetTitle(const base::string16& title) override;
  void AckConfigure() override;
  void SetWindowGeometry(const gfx::Rect& bounds) override;
  void SetAppId(const std::string& app_id) override;

  // xdg_surface_listener
  static void Configure(void* data,
                        struct xdg_surface* xdg_surface,
                        uint32_t serial);
  static void ConfigureTopLevel(void* data,
                                struct xdg_toplevel* xdg_toplevel,
                                int32_t width,
                                int32_t height,
                                struct wl_array* states);

  // xdg_toplevel_listener
  static void CloseTopLevel(void* data, struct xdg_toplevel* xdg_toplevel);

  xdg_surface* xdg_surface() const;

 private:
  WaylandWindow* wayland_window_;
  uint32_t pending_configure_serial_;
  wl::Object<struct xdg_surface> xdg_surface_;
  wl::Object<xdg_toplevel> xdg_toplevel_;

  bool surface_for_popup_ = false;

  DISALLOW_COPY_AND_ASSIGN(XDGSurfaceWrapperStable);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_XDG_SURFACE_WRAPPER_STABLE_H_

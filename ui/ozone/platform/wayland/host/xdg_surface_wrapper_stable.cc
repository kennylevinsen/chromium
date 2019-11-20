// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/xdg_surface_wrapper_stable.h"

#include <xdg-shell-client-protocol.h>

#include "base/strings/utf_string_conversions.h"
#include "ui/base/hit_test.h"
#include "ui/ozone/platform/wayland/common/wayland_util.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

namespace ui {

XDGSurfaceWrapperStable::XDGSurfaceWrapperStable(WaylandWindow* wayland_window)
    : wayland_window_(wayland_window) {}

XDGSurfaceWrapperStable::~XDGSurfaceWrapperStable() {}

bool XDGSurfaceWrapperStable::Initialize(WaylandConnection* connection,
                                         wl_surface* surface,
                                         bool with_toplevel) {
  static const xdg_surface_listener xdg_surface_listener = {
      &XDGSurfaceWrapperStable::Configure,
  };
  static const xdg_toplevel_listener xdg_toplevel_listener = {
      &XDGSurfaceWrapperStable::ConfigureTopLevel,
      &XDGSurfaceWrapperStable::CloseTopLevel,
  };

  // if this surface is created for the popup role, mark that it requires
  // configuration acknowledgement on each configure event.
  surface_for_popup_ = !with_toplevel;

  xdg_surface_.reset(xdg_wm_base_get_xdg_surface(connection->shell(), surface));
  if (!xdg_surface_) {
    LOG(ERROR) << "Failed to create xdg_surface";
    return false;
  }
  xdg_surface_add_listener(xdg_surface_.get(), &xdg_surface_listener, this);
  // XDGPopup requires a separate surface to be created, so this is just a
  // request to get an xdg_surface for it.
  if (surface_for_popup_)
    return true;

  xdg_toplevel_.reset(xdg_surface_get_toplevel(xdg_surface_.get()));
  if (!xdg_toplevel_) {
    LOG(ERROR) << "Failed to create xdg_toplevel";
    return false;
  }
  xdg_toplevel_add_listener(xdg_toplevel_.get(), &xdg_toplevel_listener, this);
  wl_surface_commit(surface);
  return true;
}

void XDGSurfaceWrapperStable::SetMaximized() {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_set_maximized(xdg_toplevel_.get());
}

void XDGSurfaceWrapperStable::UnSetMaximized() {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_unset_maximized(xdg_toplevel_.get());
}

void XDGSurfaceWrapperStable::SetFullscreen() {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_set_fullscreen(xdg_toplevel_.get(), nullptr);
}

void XDGSurfaceWrapperStable::UnSetFullscreen() {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_unset_fullscreen(xdg_toplevel_.get());
}

void XDGSurfaceWrapperStable::SetMinimized() {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_set_minimized(xdg_toplevel_.get());
}

void XDGSurfaceWrapperStable::SurfaceMove(WaylandConnection* connection) {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_move(xdg_toplevel_.get(), connection->seat(),
                    connection->serial());
}

void XDGSurfaceWrapperStable::SurfaceResize(WaylandConnection* connection,
                                            uint32_t hittest) {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_resize(xdg_toplevel_.get(), connection->seat(),
                      connection->serial(),
                      wl::IdentifyDirection(*connection, hittest));
}

void XDGSurfaceWrapperStable::SetTitle(const base::string16& title) {
  DCHECK(xdg_toplevel_);
  xdg_toplevel_set_title(xdg_toplevel_.get(), base::UTF16ToUTF8(title).c_str());
}

void XDGSurfaceWrapperStable::AckConfigure() {
  DCHECK(xdg_surface_);
  xdg_surface_ack_configure(xdg_surface_.get(), pending_configure_serial_);
}

void XDGSurfaceWrapperStable::SetWindowGeometry(const gfx::Rect& bounds) {
  DCHECK(xdg_surface_);
  xdg_surface_set_window_geometry(xdg_surface_.get(), bounds.x(), bounds.y(),
                                  bounds.width(), bounds.height());
}

void XDGSurfaceWrapperStable::SetAppId(const std::string& app_id) {
  xdg_toplevel_set_app_id(xdg_toplevel_.get(), app_id.c_str());
}

// static
void XDGSurfaceWrapperStable::Configure(void* data,
                                        struct xdg_surface* xdg_surface,
                                        uint32_t serial) {
  auto* surface = static_cast<XDGSurfaceWrapperStable*>(data);
  DCHECK(surface);
  surface->pending_configure_serial_ = serial;

  surface->AckConfigure();
}

// static
void XDGSurfaceWrapperStable::ConfigureTopLevel(
    void* data,
    struct xdg_toplevel* xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array* states) {
  auto* surface = static_cast<XDGSurfaceWrapperStable*>(data);
  DCHECK(surface);

  bool is_maximized =
      CheckIfWlArrayHasValue(states, XDG_TOPLEVEL_STATE_MAXIMIZED);
  bool is_fullscreen =
      CheckIfWlArrayHasValue(states, XDG_TOPLEVEL_STATE_FULLSCREEN);
  bool is_activated =
      CheckIfWlArrayHasValue(states, XDG_TOPLEVEL_STATE_ACTIVATED);

  surface->wayland_window_->HandleSurfaceConfigure(width, height, is_maximized,
                                                   is_fullscreen, is_activated);
}

// static
void XDGSurfaceWrapperStable::CloseTopLevel(void* data,
                                            struct xdg_toplevel* xdg_toplevel) {
  auto* surface = static_cast<XDGSurfaceWrapperStable*>(data);
  DCHECK(surface);
  surface->wayland_window_->OnCloseRequest();
}

xdg_surface* XDGSurfaceWrapperStable::xdg_surface() const {
  DCHECK(xdg_surface_);
  return xdg_surface_.get();
}

}  // namespace ui

// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/xdg_popup_wrapper_stable.h"

#include <xdg-shell-client-protocol.h>
#include <memory>

#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_pointer.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"
#include "ui/ozone/platform/wayland/host/xdg_surface_wrapper_stable.h"

namespace ui {

namespace {

constexpr uint32_t kAnchorDefaultWidth = 1;
constexpr uint32_t kAnchorDefaultHeight = 1;
constexpr uint32_t kAnchorHeightParentMenu = 30;

enum class MenuType {
  TYPE_RIGHT_CLICK,
  TYPE_3DOT_PARENT_MENU,
  TYPE_3DOT_CHILD_MENU,
  TYPE_UNKNOWN,
};

uint32_t GetAnchor(MenuType menu_type, const gfx::Rect& bounds) {
  uint32_t anchor = 0;
  switch (menu_type) {
    case MenuType::TYPE_RIGHT_CLICK:
      anchor = XDG_POSITIONER_ANCHOR_TOP | XDG_POSITIONER_ANCHOR_LEFT;
      break;
    case MenuType::TYPE_3DOT_PARENT_MENU:
      anchor = XDG_POSITIONER_ANCHOR_BOTTOM | XDG_POSITIONER_ANCHOR_RIGHT;
      break;
    case MenuType::TYPE_3DOT_CHILD_MENU:
      anchor = XDG_POSITIONER_ANCHOR_TOP;
      // Chromium may want to manually position a child menu on the left side of
      // its parent menu. Thus, react accordingly. Positive x means the child is
      // located on the right side of the parent and negative - on the left
      // side.
      if (bounds.x() >= 0)
        anchor |= XDG_POSITIONER_ANCHOR_RIGHT;
      else
        anchor |= XDG_POSITIONER_ANCHOR_LEFT;
      break;
    case MenuType::TYPE_UNKNOWN:
      NOTREACHED() << "Unsupported menu type";
      break;
  }

  return anchor;
}

uint32_t GetGravity(MenuType menu_type, const gfx::Rect& bounds) {
  uint32_t gravity = 0;
  switch (menu_type) {
    case MenuType::TYPE_RIGHT_CLICK:
      gravity = XDG_POSITIONER_GRAVITY_BOTTOM | XDG_POSITIONER_GRAVITY_RIGHT;
      break;
    case MenuType::TYPE_3DOT_PARENT_MENU:
      gravity = XDG_POSITIONER_GRAVITY_BOTTOM | XDG_POSITIONER_GRAVITY_RIGHT;
      break;
    case MenuType::TYPE_3DOT_CHILD_MENU:
      gravity = XDG_POSITIONER_GRAVITY_BOTTOM;
      // Chromium may want to manually position a child menu on the left side of
      // its parent menu. Thus, react accordingly. Positive x means the child is
      // located on the right side of the parent and negative - on the left
      // side.
      if (bounds.x() >= 0)
        gravity |= XDG_POSITIONER_GRAVITY_RIGHT;
      else
        gravity |= XDG_POSITIONER_GRAVITY_LEFT;
      break;
    case MenuType::TYPE_UNKNOWN:
      NOTREACHED() << "Unsupported menu type";
      break;
  }

  return gravity;
}

uint32_t GetConstraintAdjustment(MenuType menu_type) {
  uint32_t constraint = 0;

  switch (menu_type) {
    case MenuType::TYPE_RIGHT_CLICK:
      constraint = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
                   XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y |
                   XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y;
      break;
    case MenuType::TYPE_3DOT_PARENT_MENU:
      constraint = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
                   XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y;
      break;
    case MenuType::TYPE_3DOT_CHILD_MENU:
      constraint = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X |
                   XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
      break;
    case MenuType::TYPE_UNKNOWN:
      NOTREACHED() << "Unsupported menu type";
      break;
  }

  return constraint;
}

gfx::Rect GetAnchorRect(MenuType menu_type,
                        const gfx::Rect& menu_bounds,
                        const gfx::Rect& parent_window_bounds) {
  gfx::Rect anchor_rect;
  switch (menu_type) {
    case MenuType::TYPE_RIGHT_CLICK:
      // Place anchor for right click menus normally.
      anchor_rect = gfx::Rect(menu_bounds.x(), menu_bounds.y(),
                              kAnchorDefaultWidth, kAnchorDefaultHeight);
      break;
    case MenuType::TYPE_3DOT_PARENT_MENU:
      // The anchor for parent menu windows is positioned slightly above the
      // specified bounds to ensure flipped window along y-axis won't hide 3-dot
      // menu button.
      anchor_rect = gfx::Rect(menu_bounds.x() - kAnchorDefaultWidth,
                              menu_bounds.y() - kAnchorHeightParentMenu,
                              kAnchorDefaultWidth, kAnchorHeightParentMenu);
      break;
    case MenuType::TYPE_3DOT_CHILD_MENU:
      // The child menu's anchor must meet the following requirements: at some
      // point, the Wayland compositor can flip it along x-axis. To make sure
      // it's positioned correctly, place it closer to the beginning of the
      // parent menu shifted by the same value along x-axis. The width of anchor
      // must correspond the width between two points - specified origin by the
      // Chromium and calculated point shifted by the same value along x-axis
      // from the beginning of the parent menu width.
      //
      // We also have to bear in mind that Chromium may decide to flip the
      // position of the menu window along the x-axis and show it on the other
      // side of the parent menu window (normally, the Wayland compositor does
      // it). Thus, check which side the child menu window is going to be
      // presented on and create right anchor.
      if (menu_bounds.x() >= 0) {
        auto anchor_width =
            parent_window_bounds.width() -
            (parent_window_bounds.width() - menu_bounds.x()) * 2;
        anchor_rect =
            gfx::Rect(parent_window_bounds.width() - menu_bounds.x(),
                      menu_bounds.y(), anchor_width, kAnchorDefaultHeight);
      } else {
        DCHECK_LE(menu_bounds.x(), 0);
        auto position = menu_bounds.width() + menu_bounds.x();
        DCHECK(position > 0 && position < parent_window_bounds.width());
        auto anchor_width = parent_window_bounds.width() - position * 2;
        anchor_rect = gfx::Rect(position, menu_bounds.y(), anchor_width,
                                kAnchorDefaultHeight);
      }
      break;
    case MenuType::TYPE_UNKNOWN:
      NOTREACHED() << "Unsupported menu type";
      break;
  }

  return anchor_rect;
}

}  // namespace

XDGPopupWrapperStable::XDGPopupWrapperStable(
    std::unique_ptr<XDGSurfaceWrapper> surface,
    WaylandWindow* wayland_window)
    : wayland_window_(wayland_window), xdg_surface_(std::move(surface)) {
  DCHECK(xdg_surface_);
}

XDGPopupWrapperStable::~XDGPopupWrapperStable() {}

bool XDGPopupWrapperStable::Initialize(WaylandConnection* connection,
                                       wl_surface* surface,
                                       WaylandWindow* parent_window,
                                       const gfx::Rect& bounds) {
  DCHECK(connection && surface && parent_window);
  static const struct xdg_popup_listener xdg_popup_listener = {
      &XDGPopupWrapperStable::Configure,
      &XDGPopupWrapperStable::PopupDone,
  };

  XDGSurfaceWrapperStable* xdg_surface =
      static_cast<XDGSurfaceWrapperStable*>(xdg_surface_.get());
  if (!xdg_surface)
    return false;

  XDGSurfaceWrapperStable* parent_xdg_surface;
  // If the parent window is a popup, the surface of that popup must be used as
  // a parent.
  if (parent_window->xdg_popup()) {
    XDGPopupWrapperStable* popup =
        reinterpret_cast<XDGPopupWrapperStable*>(parent_window->xdg_popup());
    parent_xdg_surface =
        reinterpret_cast<XDGSurfaceWrapperStable*>(popup->xdg_surface());
  } else {
    parent_xdg_surface = reinterpret_cast<XDGSurfaceWrapperStable*>(
        parent_window->xdg_surface());
  }

  if (!parent_xdg_surface)
    return false;

  struct xdg_positioner* positioner =
      CreatePositioner(connection, parent_window, bounds);
  if (!positioner)
    return false;

  xdg_popup_.reset(xdg_surface_get_popup(xdg_surface->xdg_surface(),
                                         parent_xdg_surface->xdg_surface(),
                                         positioner));
  if (!xdg_popup_)
    return false;

  xdg_positioner_destroy(positioner);

  // According to the spec, the grab call can only be done on a key press, mouse
  // press or touch down. However, there is a problem with popup windows and
  // touch events so long as Chromium creates them only on consequent touch up
  // events. That results in Wayland compositors dismissing popups. To fix
  // the issue, do not use grab with touch events. Please note that explicit
  // grab means that a Wayland compositor dismisses a popup whenever the user
  // clicks outside the created surfaces. If the explicit grab is not used, the
  // popups are not dismissed in such cases. What is more, current non-ozone X11
  // implementation does the same. This means there is no functionality changes
  // and we do things right.
  //
  // We cannot know what was the last event. Instead, we can check if the window
  // has pointer or keyboard focus. If so, the popup will be explicitly grabbed.
  //
  // There is a bug in the gnome/mutter - if explicit grab is not used,
  // unmapping of a wl_surface (aka destroying xdg_popup and surface) to hide a
  // window results in weird behaviour. That is, a popup continues to be visible
  // on a display and it results in a crash of the entire session. Thus, just
  // continue to use grab here and avoid showing popups for touch events on
  // gnome/mutter. That is better than crashing the entire system. Otherwise,
  // Chromium has to change the way how it reacts on touch events - instead of
  // creating a menu on touch up, it must do it on touch down events.
  // https://gitlab.gnome.org/GNOME/mutter/issues/698#note_562601.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if ((base::nix::GetDesktopEnvironment(env.get()) ==
       base::nix::DESKTOP_ENVIRONMENT_GNOME) ||
      (wayland_window_->parent_window()->has_pointer_focus() ||
       wayland_window_->parent_window()->has_keyboard_focus())) {
    xdg_popup_grab(xdg_popup_.get(), connection->seat(), connection->serial());
  }
  xdg_popup_add_listener(xdg_popup_.get(), &xdg_popup_listener, this);

  wl_surface_commit(surface);
  return true;
}

struct xdg_positioner* XDGPopupWrapperStable::CreatePositioner(
    WaylandConnection* connection,
    WaylandWindow* parent_window,
    const gfx::Rect& bounds) {
  struct xdg_positioner* positioner;
  positioner = xdg_wm_base_create_positioner(connection->shell());
  if (!positioner)
    return nullptr;

  auto* pointer = connection->pointer();
  uint32_t flags = 0;
  if (pointer)
    flags = pointer->GetFlagsWithKeyboardModifiers();
  bool is_right_click_menu = flags & EF_RIGHT_MOUSE_BUTTON;

  // Different types of menu require different anchors, constraint adjustments,
  // gravity and etc.
  MenuType menu_type = MenuType::TYPE_UNKNOWN;
  if (is_right_click_menu)
    menu_type = MenuType::TYPE_RIGHT_CLICK;
  else if (parent_window->xdg_popup())
    menu_type = MenuType::TYPE_3DOT_CHILD_MENU;
  else
    menu_type = MenuType::TYPE_3DOT_PARENT_MENU;

  // Place anchor to the end of the possible position.
  gfx::Rect anchor_rect = GetAnchorRect(
      menu_type, bounds,
      gfx::ScaleToRoundedRect(parent_window->GetBounds(),
                              1.0 / parent_window->buffer_scale()));

  xdg_positioner_set_anchor_rect(positioner, anchor_rect.x(), anchor_rect.y(),
                                 anchor_rect.width(), anchor_rect.height());
  xdg_positioner_set_size(positioner, bounds.width(), bounds.height());
  xdg_positioner_set_anchor(positioner, GetAnchor(menu_type, bounds));
  xdg_positioner_set_gravity(positioner, GetGravity(menu_type, bounds));
  xdg_positioner_set_constraint_adjustment(positioner,
                                           GetConstraintAdjustment(menu_type));
  return positioner;
}

// static
void XDGPopupWrapperStable::Configure(void* data,
                                      struct xdg_popup* xdg_popup,
                                      int32_t x,
                                      int32_t y,
                                      int32_t width,
                                      int32_t height) {
  // As long as the Wayland compositor repositions/requires to position windows
  // relative to their parents, do not propagate final bounds information to
  // Chromium. The browser places windows in respect to screen origin, but
  // Wayland requires doing so in respect to parent window's origin. To properly
  // place windows, the bounds are translated and adjusted according to the
  // Wayland compositor needs during WaylandWindow::CreateXdgPopup call.
  WaylandWindow* window =
      static_cast<XDGPopupWrapperStable*>(data)->wayland_window_;
  DCHECK(window);
  window->HandlePopupConfigure({x, y, width, height});
}

// static
void XDGPopupWrapperStable::PopupDone(void* data, struct xdg_popup* xdg_popup) {
  WaylandWindow* window =
      static_cast<XDGPopupWrapperStable*>(data)->wayland_window_;
  DCHECK(window);
  window->Hide();
  window->OnCloseRequest();
}

XDGSurfaceWrapper* XDGPopupWrapperStable::xdg_surface() {
  DCHECK(xdg_surface_.get());
  return xdg_surface_.get();
}

}  // namespace ui

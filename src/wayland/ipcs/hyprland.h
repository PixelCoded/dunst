/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * @file
 * @ingroup wayland
 * @brief Wayland output wrapper
 * @copyright Copyright 2021-2026 Dunst contributors
 * @license BSD-3-Clause
 */

#ifndef DUNST_HYPRLAND_H
#define DUNST_HYPRLAND_H

#include <stdbool.h>
#include <glib.h>

struct wl_hyprland_ipc {
        int socket2_fd;
        struct dunst_output *focused_monitor;
        GSource *source;
};

extern struct wl_hyprland_ipc hyprland_ipc;

bool wl_hyprland_init(void);
struct dunst_output *wl_hyprland_get_focused_output(void);
bool wl_hyprland_update(void);

#endif // DUNST_HYPRLAND_H_

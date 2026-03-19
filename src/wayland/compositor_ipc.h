/* SPDX-License-Identifier: BSD-3-Clause */
/**
 * @file
 * @ingroup wayland
 * @brief Wayland output wrapper
 * @copyright Copyright 2021-2026 Dunst contributors
 * @license BSD-3-Clause
 */

#ifndef DUNST_WM_H
#define DUNST_WM_H

#include <stdbool.h>
#include "wl_output.h"

struct wl_compositor_ipc {
        char *name;

        bool(*init)(void); // Init function. Cannot be NULL.
        struct dunst_output*(*get_focused_output)(void); // Can be NULL, if the IPC doesn't support this.
        bool(*update)(void);
};

struct wl_compositor_ipc *wl_get_compositor_ipc(void);

#endif // DUNST_WM_H

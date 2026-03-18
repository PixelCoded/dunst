#ifndef DUNST_HYPRLAND_H
#define DUNST_HYPRLAND_H

#include <stdbool.h>
#include <glib.h>

struct wl_hyprland_context {
        int socket2_fd;
        struct dunst_output *focused_monitor;
        GSource *source;
};

extern struct wl_hyprland_context hyprland_ctx;

bool wl_hyprland_init(void);
struct dunst_output *wl_hyprland_get_active_screen(void);
bool wl_hyprland_update(void);

#endif // DUNST_HYPRLAND_H_

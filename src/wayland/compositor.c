#define _GNU_SOURCE
#include "compositor.h"
#include "wl_ctx.h"
#include "../log.h"

#include <errno.h>
#include <glib.h>

#include <sys/socket.h>

#include "compositors/hyprland.h"

struct compositor supported_compositors[] = {
        (struct compositor) {
                .name              = "hyprland",
                .init              = wl_hyprland_init,
                .get_active_screen = wl_hyprland_get_active_screen,
                .update            = wl_hyprland_update,
        }
};

struct compositor *wl_get_compositor() {
        int fd = wl_display_get_fd(ctx.display);

        struct ucred ucred;
        socklen_t len = sizeof(struct ucred);
        if(getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
                LOG_E("Coulnd't getsockopt Wayland socket. Error being %s\n", strerror(errno));
                return NULL;
        }

        int proc_path_len = g_snprintf(NULL, 0, "/proc/%d/comm", ucred.pid) + 1;
        char proc_path[proc_path_len];
        g_snprintf(proc_path, proc_path_len, "/proc/%d/comm", ucred.pid);

        FILE *proc_name_file = fopen(proc_path, "r");
        if(proc_name_file == NULL) {
                LOG_E("Couldn't open %.*s!", proc_path_len - 1, proc_path);
                return NULL;
        }

        char *compositor_name;
        size_t compositor_name_len = 0;
        if(getline(&compositor_name, &compositor_name_len, proc_name_file) == -1) {
                LOG_E("Couldn't read %.*s!", proc_path_len - 1, proc_path);
                return NULL;
        }

        for(size_t i = 0; i < sizeof(supported_compositors) / sizeof(supported_compositors[0]); i++) {
                if(g_strcmp0(supported_compositors[i].name, compositor_name)) {
                        LOG_D("Found supported Wayland compositor \"%s\" with pid %d", supported_compositors[i].name, ucred.pid);
                        return &supported_compositors[i];
                }
        }

        LOG_E("No supported Wayland compositors found!");
        return NULL;
}

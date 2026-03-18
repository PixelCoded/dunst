#include "hyprland.h"
#include "../../log.h"
#include "../wl_ctx.h"
#include "../wl_output.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <poll.h>
#include <wayland-util.h>

struct wl_hyprland_source {
        GSource source;
};
struct wl_hyprland_context hyprland_ctx = { 0 };

static char *wl_hyprland_get_sockpath(char *name);
static char *wl_hyprland_readuntil(int fd, char *until);
static bool wl_hyprland_set_focused(char *mon_name);
static GSource *wl_hyprland_register_source(int fd);

bool wl_hyprland_init(void) {
        char *socket2_path = wl_hyprland_get_sockpath("socket2");
        if(socket2_path == NULL) {
                LOG_E("Couldn't get socket address for Hyprlan'ds socket2!");
                return false;
        }

        int socket2_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(socket2_fd == -1) {
                return false; // TODO: Error message.
        }

        struct sockaddr_un socket2_addr = {
                .sun_family = AF_UNIX,
        };

        memmove(socket2_addr.sun_path, socket2_path, sizeof(socket2_addr.sun_path));
        if(connect(socket2_fd, (struct sockaddr*)&socket2_addr, sizeof(socket2_addr)) == -1) {
                LOG_E("Couldn't connect to Hyprland's socket2 socket due to %s", strerror(errno));
                close(socket2_fd);
                return false;
        }

        int flags = fcntl(socket2_fd, F_GETFL);
        if(flags == -1 || fcntl(socket2_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                LOG_E("Couldn't set Hyprland's socket2 socket to be non-blocking due to %s", strerror(errno));
                close(socket2_fd);
                return false;
        }

        hyprland_ctx.socket2_fd = socket2_fd;
        hyprland_ctx.source     = wl_hyprland_register_source(socket2_fd);
        LOG_D("Succesfully connected to Hyprland's socket2 socket");
        while(wl_hyprland_update()) {
                /* Do nothing. Flush all events sent to us by Hyprland. */
        }
        return true;
}

struct dunst_output *wl_hyprland_get_active_screen(void) {
        wl_hyprland_update();
        return hyprland_ctx.focused_monitor;
}

bool wl_hyprland_update(void) {
        char *cmd = wl_hyprland_readuntil(hyprland_ctx.socket2_fd, ">>");
        if(cmd == NULL) {
                return false;
        }

        LOG_D("CMD: %s", cmd);
        if(g_strcmp0(cmd, "focusedmon")) {
                char *mon_name = wl_hyprland_readuntil(hyprland_ctx.socket2_fd, ",");
                /*char *workspace_name =*/ wl_hyprland_readuntil(hyprland_ctx.socket2_fd, "\n");
                wl_hyprland_set_focused(mon_name);
        } else if(g_strcmp0(cmd, "focusedmonv2")) {
                char *mon_name = wl_hyprland_readuntil(hyprland_ctx.socket2_fd, ",");
                /*char *workspace_id = */ wl_hyprland_readuntil(hyprland_ctx.socket2_fd, "\n");
                wl_hyprland_set_focused(mon_name);
        } else {
                wl_hyprland_readuntil(hyprland_ctx.socket2_fd, "\n");
        }
        return true;
}

static char *wl_hyprland_get_sockpath(char *name) {
        char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
        if(xdg_runtime_dir == NULL) {
                LOG_C("No environment variable XDG_RUNTIME_DIR is set!");
                return NULL;
        }

        char *his = getenv("HYPRLAND_INSTANCE_SIGNATURE");
        if(his == NULL) {
                LOG_C("No environment variable HYPRLAND_INSTANCE_SIGNATURE is set!");
                return NULL;
        }

        int len = g_snprintf(NULL, 0, "%s/hypr/%s/.%s.sock", xdg_runtime_dir, his, name) + 1;
        char *buffer = g_malloc(len);
        g_snprintf(buffer, len, "%s/hypr/%s/.%s.sock", xdg_runtime_dir, his, name);
        return buffer;
}

static char *wl_hyprland_readuntil(int fd, char *until) {
        #define BUFFER_SIZE 1024
        static char BUFFER[BUFFER_SIZE];

        size_t len        = strlen(until);
        size_t buffer_pos = 0;
        for(size_t i = 0; i < len;) {
                char read_char = '\0';
                if(read(fd, &read_char, sizeof(read_char)) <= 0 || buffer_pos + 1 >= BUFFER_SIZE) {
                        return NULL;
                }

                if(until[i] == read_char) {
                        i++;
                } else if(i != 0) {
                        i = 0;
                }
                BUFFER[buffer_pos++] = read_char;
        }
        if(buffer_pos <= len) {
                return NULL;
        }
        memset(&BUFFER[buffer_pos - len], '\0', len);
        #undef BUFFER_SIZE
        return BUFFER;
}

static bool wl_hyprland_set_focused(char *mon_name) {
        struct dunst_output *output;
        wl_list_for_each(output, &ctx.outputs, link) {
                if(g_strcmp0(output->name, mon_name)) {
                        // TODO: Not sure whetever mon_name is output->name or output->global_name
                        hyprland_ctx.focused_monitor = output;
                        return true;
                }
        }
        return false;
}

static gboolean wl_hyprland_fd_prepare(GSource *source, gint *timeout) {
        if(timeout) {
                *timeout = -1;
        }
        return false;
}

static gboolean wl_hyprland_fd_dispatch(GSource *source, GSourceFunc callback, gpointer user) {
        while(wl_hyprland_update()) {
                // Do nothing.
        }
        return G_SOURCE_CONTINUE;
}

static gboolean wl_hyprland_fd_check(GSource *source) {
        struct pollfd pollfd = {
                .fd = hyprland_ctx.socket2_fd,
                .events = POLLIN,
        };
        return poll(&pollfd, 1, 0) > 0;
}

static GSource *wl_hyprland_register_source(int fd) {
        static GSourceFuncs wl_hyprland_fn = {
                .prepare          = wl_hyprland_fd_prepare,
                .check            = wl_hyprland_fd_check,
                .dispatch         = wl_hyprland_fd_dispatch,
                .finalize         = NULL,
                .closure_callback = NULL,
                .closure_marshal  = NULL,
        };

        struct wl_hyprland_source *source = (struct wl_hyprland_source*)g_source_new(&wl_hyprland_fn,sizeof(*source));
        g_source_add_unix_fd((GSource*)source, fd, G_IO_IN);
        g_source_attach((GSource*)source, NULL);
        return (GSource*)source;
}

#ifndef DUNST_WM_H
#define DUNST_WM_H

#include <stdbool.h>
#include "wl_output.h"

struct compositor {
        char *name;
        bool(*init)(void);
        struct dunst_output*(*get_active_screen)(void);
        bool(*update)(void);
};

struct compositor *wl_get_compositor(void);

#endif // DUNST_WM_H

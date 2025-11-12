#ifndef CODEVAULT_H
#define CODEVAULT_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

static inline const char *cv_resolve_vault_dir(const char *override) {
    if (override and override[0]) return override;

    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg and *xdg) return xdg;

    const char *home = getenv("HOME");
    if (home and *home) return home;

    return ".";
}

#endif
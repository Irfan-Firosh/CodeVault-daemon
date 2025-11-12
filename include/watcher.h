#ifndef WATCHER_H
#define WATCHER_H

#include <CoreServices/CoreServices.h>

typedef void (*cv_repo_callback)(const char *path);

int cv_watcher_start(const char *path, cv_repo_callback cb);

void cv_watcher_stop(void);

#endif
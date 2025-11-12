#include "watcher.h"
#include <stdio.h>

void on_repo_changed(const char *path) {
    printf("[EVENT] Change detected: %s\n", path);
}

int main() {
    const char *repo_path = "/Users/irfanfirosh/Documents/Personal projects/CodeVault/daemon";
    printf("Starting watcher on %s...\n", repo_path);
    return cv_watcher_start(repo_path, on_repo_changed);
}
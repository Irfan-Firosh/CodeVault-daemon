#include "codevault.h"
#include "db.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;

static void handle_sig(int sig) {
    if (sig == SIGINT || sig == SIGTERM) running = 0;
}

static int daemonize_process(bool foreground) {
    if (foreground) return 0;
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) {
        printf("codevault-daemon started (pid=%d)\n", pid);
        fflush(stdout);
        _exit(0);
    }
    if (setsid() < 0) return -1;
    // second fork
    pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) _exit(0);
    umask(0);
    if (chdir("/") != 0) {/* tolerable for not switching to root dir avoid locking mount*/}

    int fd0 = open("/dev/null", O_RDONLY);
    int fd1 = open("/dev/null", O_WRONLY);
    int fd2 = open("/dev/null", O_WRONLY);

    if (fd0 >= 0) dup2(fd0, STDIN_FILENO);
    if (fd1 >= 0) dup2(fd1, STDOUT_FILENO);
    if (fd2 >= 0) dup2(fd2, STDERR_FILENO);

    if (fd0 > 2) close(fd0);
    if (fd1 > 2) close(fd1);
    if (fd2 > 2) close(fd2);

    return 0;
}

int main(int argc, char **argv) {
    bool foreground = false;
    bool init_db_only = false;
    const char *vault_override = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--foreground") == 0) foreground=true;
        else if (strcmp(argv[i], "--inti-db") == 0) init_db_only=true;
        else if (strncmp(argv[i], "--vault-dir=", 12) == 0) vault_override = argv[i] + 12;
    }

    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    if (daemonize_process(foreground)  != 0) {
        fprintf(stderr, "daemonize failed: %s\n", strerror(errno));
        return 1;
    }

    const char *base = cv_resolve_vault_dir(vault_override);
    char dirpath[PATH_MAX];
    char dbpath[PATH_MAX];
    snprintf(dirpath, sizeof(dirpath), "%s/.codevault", base);

    if (mkdir(dirpath, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "mkdir %s failed: %s\n", dirpath, strerror(errno));
        return 1;
    }
    snprintf(dbpath, sizeof(dbpath), "%s/vault.db", dirpath);

    cv_db_t db = {0};

    if (cv_db_open(&db, dbpath) != SQLITE_OK) {
        fprintf(stderr, "Failed to open DB: %s\n", dbpath);
        return 1;
    }

    if (cv_db_init_schema(&db) != SQLITE_OK) {
        fprintf(stderr, "Failed to intialize DB schema\n");
        cv_db_close(&db);
        return 1;
    }

    if (init_db_only) {
        cv_db_close(&db);
        return 0;
    }

    while (running) {
        // actual code goes here
        sleep(1);
    }

    cv_db_close(&db);
    return 0;
}
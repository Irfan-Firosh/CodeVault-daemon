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
}
/*
 * setcap_netmon.c
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>

static void print_usage(const char *progname) {
    printf("Usage:\n");
    printf("  %s [target]    Apply cap_net_raw=eip to target binary (default: build/netmon)\n", progname);
    printf("  %s --clean     Remove this setuid helper binary using root privileges\n", progname);
    printf("  %s --help      Display this help message\n", progname);
}

static int do_clean(const char *progname) {
    char self_path[PATH_MAX];
    memset(self_path, 0, sizeof(self_path));

    ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len > 0) {
        self_path[len] = '\0';
        if (unlink(self_path) == 0) {
            printf("[setcap_netmon] Successfully removed %s\n", self_path);
            return 0;
        }
    }

    if (unlink(progname) == 0) {
        printf("[setcap_netmon] Successfully removed %s\n", progname);
        return 0;
    }

    perror("[setcap_netmon] Failed to remove helper binary");
    return 1;
}

static int apply_capability(const char *target) {
    struct stat st;
    if (stat(target, &st) != 0) {
        fprintf(stderr, "[setcap_netmon] Error: target '%s' not found\n", target);
        return 1;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "[setcap_netmon] Error: target '%s' is not a regular file\n", target);
        return 1;
    }

    // Safety constraint: Target must be named "netmon"
    size_t tlen = strlen(target);
    if (tlen < 6) {
        fprintf(stderr, "[setcap_netmon] Error: target must be 'netmon'\n");
        return 1;
    }
    if (strcmp(target + tlen - 6, "netmon") != 0) {
        fprintf(stderr, "[setcap_netmon] Error: target '%s' does not end with 'netmon'\n", target);
        return 1;
    }
    if (tlen > 6 && target[tlen - 7] != '/') {
        fprintf(stderr, "[setcap_netmon] Error: target '%s' is not a valid 'netmon' binary\n", target);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("[setcap_netmon] fork failed");
        return 1;
    }

    if (pid == 0) {
        char *setcap_args[] = {
            (char *)"setcap",
            (char *)"cap_net_raw=eip",
            (char *)target,
            NULL
        };

        execv("/usr/sbin/setcap", setcap_args);
        execv("/sbin/setcap", setcap_args);
        execvp("setcap", setcap_args);
        perror("[setcap_netmon] execv setcap failed");
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("[setcap_netmon] waitpid failed");
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("[setcap_netmon] Successfully applied cap_net_raw=eip to %s\n", target);
        return 0;
    }

    fprintf(stderr, "[setcap_netmon] Error: setcap failed with exit code %d\n",
            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 1;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Check effective UID
    uid_t euid = geteuid();
    if (euid != 0) {
        fprintf(stderr, "[setcap_netmon] Error: helper is not running as root (euid=%d).\n", (int)euid);
        fprintf(stderr, "[setcap_netmon] Run once: sudo chown root:root %s && sudo chmod 4755 %s\n",
                argv[0], argv[0]);
        return 1;
    }

    // Elevate real IDs to root
    if (setgid(0) != 0) {
        perror("[setcap_netmon] setgid(0) failed");
    }
    if (setuid(0) != 0) {
        perror("[setcap_netmon] setuid(0) failed");
    }

    if (argc > 1 && strcmp(argv[1], "--clean") == 0) {
        return do_clean(argv[0]);
    }

    const char *target = (argc > 1) ? argv[1] : "build/netmon";
    return apply_capability(target);
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

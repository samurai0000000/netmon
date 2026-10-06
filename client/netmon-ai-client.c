/*
 * netmon-ai-client.c
 *
 * Copyright (C) 2026, Charles Chiou
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define MAX_REQ_LEN       4096
#define MAX_RSP_LEN       1048576
#define IDLE_TIMEOUT_SEC  300
#define DEFAULT_ENDPOINT  "127.0.0.1:3885"

static int write_all(int fd, const void *buf, size_t count) {
    const uint8_t *ptr = (const uint8_t *)buf;
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        ptr += n;
        remaining -= (size_t)n;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t count) {
    uint8_t *ptr = (uint8_t *)buf;
    size_t remaining = count;
    while (remaining > 0) {
        ssize_t n = read(fd, ptr, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return 0; // EOF
        }
        ptr += n;
        remaining -= (size_t)n;
    }
    return 1; // Completed
}

static int send_msg(int fd, const char *msg) {
    size_t len = strlen(msg);
    if (len == 0 || len > MAX_RSP_LEN) {
        return -1;
    }
    uint32_t len_be = htonl((uint32_t)len);
    if (write_all(fd, &len_be, 4) != 0) {
        return -1;
    }
    if (write_all(fd, msg, len) != 0) {
        return -1;
    }
    return 0;
}

static int recv_msg(int fd, char **out_buf, uint32_t *out_len) {
    uint32_t len_be = 0;
    int r = read_all(fd, &len_be, 4);
    if (r <= 0) {
        return -1;
    }
    uint32_t len = ntohl(len_be);
    if (len == 0 || len > MAX_RSP_LEN) {
        return -1;
    }
    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        return -1;
    }
    r = read_all(fd, buf, len);
    if (r <= 0) {
        free(buf);
        return -1;
    }
    buf[len] = '\0';
    *out_buf = buf;
    if (out_len) {
        *out_len = len;
    }
    return 0;
}

static pid_t parse_ppid_from_stat(pid_t pid) {
    if (pid <= 0) {
        return 0;
    }
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", (int)pid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;
    }
    char buf[1024];
    if (!fgets(buf, sizeof(buf), fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    char *rparen = strrchr(buf, ')');
    if (!rparen) {
        return 0;
    }
    char state = ' ';
    int ppid = 0;
    if (sscanf(rparen + 1, " %c %d", &state, &ppid) == 2 && ppid > 0) {
        return (pid_t)ppid;
    }
    return 0;
}

static pid_t get_anchor_pid(void) {
    const char *env = getenv("NETMON_ANCHOR_PID");
    if (env && *env) {
        long v = strtol(env, NULL, 10);
        if (v > 0) {
            return (pid_t)v;
        }
    }

    pid_t shell_pid = getppid();
    pid_t anchor = parse_ppid_from_stat(shell_pid);
    if (anchor > 0) {
        return anchor;
    }
    return shell_pid;
}

static void get_sock_path(char *out_path, size_t max_len, pid_t anchor) {
    snprintf(out_path, max_len, "/tmp/netmon-ai-%d.sock", (int)anchor);
}

static int connect_tcp(const char *endpoint) {
    char host[256];
    char port_str[32];
    const char *colon = strrchr(endpoint, ':');
    if (!colon) {
        return -1;
    }
    size_t host_len = (size_t)(colon - endpoint);
    if (host_len >= sizeof(host)) {
        return -1;
    }
    memcpy(host, endpoint, host_len);
    host[host_len] = '\0';

    strncpy(port_str, colon + 1, sizeof(port_str) - 1);
    port_str[sizeof(port_str) - 1] = '\0';

    struct addrinfo hints;
    struct addrinfo *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &result) != 0 || !result) {
        return -1;
    }

    int fd = -1;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }
        int flag = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);
    return fd;
}

static uint64_t monotonic_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec;
}

static void run_daemon(pid_t anchor, const char *endpoint, const char *sock_path) {
    // Daemonize
    setsid();
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        if (null_fd > 2) {
            close(null_fd);
        }
    }

    signal(SIGPIPE, SIG_IGN);
    umask(0077); // Socket mode 0600

    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);
    unlink(sock_path);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(listen_fd);
        exit(1);
    }
    if (chmod(sock_path, 0600) != 0) {
        unlink(sock_path);
        close(listen_fd);
        exit(1);
    }
    if (listen(listen_fd, 16) != 0) {
        unlink(sock_path);
        close(listen_fd);
        exit(1);
    }

    int tcp_fd = connect_tcp(endpoint);
    if (tcp_fd < 0) {
        unlink(sock_path);
        close(listen_fd);
        exit(1);
    }

    // Protocol handshake
    if (send_msg(tcp_fd, "HELLO 1") != 0) {
        close(tcp_fd);
        unlink(sock_path);
        close(listen_fd);
        exit(1);
    }
    char *hello_rsp = NULL;
    if (recv_msg(tcp_fd, &hello_rsp, NULL) != 0 || strcmp(hello_rsp, "HELLO 1") != 0) {
        if (hello_rsp) free(hello_rsp);
        close(tcp_fd);
        unlink(sock_path);
        close(listen_fd);
        exit(1);
    }
    free(hello_rsp);

    bool is_granted = false;
    uint32_t grant_seconds = 0;
    uint64_t grant_timestamp = 0;
    bool request_pending = false;
    char pending_conn_id[64] = {0};

    uint64_t last_activity = monotonic_time_sec();

    while (1) {
        struct pollfd pfd[2];
        pfd[0].fd = listen_fd;
        pfd[0].events = POLLIN;
        pfd[0].revents = 0;

        pfd[1].fd = tcp_fd;
        pfd[1].events = POLLIN;
        pfd[1].revents = 0;

        int poll_res = poll(pfd, 2, 10000); // 10s poll
        if (poll_res < 0) {
            if (errno == EINTR) continue;
            break;
        }

        uint64_t now = monotonic_time_sec();
        if (now >= last_activity + IDLE_TIMEOUT_SEC) {
            // Idle timeout
            send_msg(tcp_fd, "CLOSE");
            break;
        }

        // Check for unsolicited remote disconnect
        if (pfd[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        }
        if (pfd[1].revents & POLLIN) {
            // Unexpected packet from router worker or server disconnect
            char *remote_msg = NULL;
            if (recv_msg(tcp_fd, &remote_msg, NULL) != 0) {
                break;
            }
            if (remote_msg) free(remote_msg);
        }

        if (pfd[0].revents & POLLIN) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd < 0) {
                continue;
            }

            // SO_PEERCRED authentication
            struct ucred cred;
            socklen_t ucred_len = sizeof(cred);
            if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &ucred_len) != 0 ||
                cred.uid != getuid()) {
                close(client_fd);
                continue;
            }

            // Check ancestry up to 8 levels
            bool authorized = (cred.pid == anchor);
            pid_t cur_ancestor = cred.pid;
            for (int d = 0; d < 8 && !authorized && cur_ancestor > 1; ++d) {
                pid_t p = parse_ppid_from_stat(cur_ancestor);
                if (p == anchor) {
                    authorized = true;
                    break;
                }
                if (p <= 1 || p == cur_ancestor) break;
                cur_ancestor = p;
            }

            const char *env_anchor = getenv("NETMON_ANCHOR_PID");
            if (!authorized && env_anchor && *env_anchor) {
                if ((pid_t)strtol(env_anchor, NULL, 10) == anchor) {
                    authorized = true;
                }
            }

            if (!authorized) {
                close(client_fd);
                continue;
            }

            struct timeval tv;
            tv.tv_sec = 30;
            tv.tv_usec = 0;
            setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            bool should_exit = false;
            while (1) {
                // Read command from front-end
                char *req = NULL;
                if (recv_msg(client_fd, &req, NULL) != 0 || !req) {
                    break;
                }

                if (strncmp(req, "REQUEST", 7) == 0) {
                    if (is_granted) {
                        uint64_t elapsed = now - grant_timestamp;
                        uint32_t rem = (elapsed >= grant_seconds) ? 0 : (grant_seconds - (uint32_t)elapsed);
                        char buf[64];
                        snprintf(buf, sizeof(buf), "ALREADY_GRANTED %u", rem);
                        send_msg(client_fd, buf);
                    } else if (request_pending) {
                        char buf[128];
                        snprintf(buf, sizeof(buf), "WAITING %s", pending_conn_id);
                        send_msg(client_fd, buf);
                    } else {
                        if (send_msg(tcp_fd, req) == 0) {
                            request_pending = true;
                            while (1) {
                                char *rsp = NULL;
                                if (recv_msg(tcp_fd, &rsp, NULL) != 0 || !rsp) {
                                    send_msg(client_fd, "FAILED - Server disconnected");
                                    break;
                                }
                                if (strncmp(rsp, "WAITING", 7) == 0) {
                                    strncpy(pending_conn_id, rsp + 7, sizeof(pending_conn_id) - 1);
                                    send_msg(client_fd, rsp);
                                    free(rsp);
                                    continue;
                                }
                                if (strncmp(rsp, "GRANTED", 7) == 0) {
                                    is_granted = true;
                                    request_pending = false;
                                    grant_timestamp = monotonic_time_sec();
                                    const char *p = rsp + 7;
                                    while (*p == ' ') p++;
                                    // Skip tier name if present (e.g. READ, READ_WRITE)
                                    while (*p && !isspace((unsigned char)*p) && !isdigit((unsigned char)*p)) {
                                        p++;
                                    }
                                    while (*p == ' ') p++;
                                    if (*p >= '0' && *p <= '9') {
                                        grant_seconds = (uint32_t)strtoul(p, NULL, 10);
                                    } else {
                                        grant_seconds = 0;
                                    }
                                    send_msg(client_fd, rsp);
                                    free(rsp);
                                    break;
                                }
                                // Rejection or failure
                                request_pending = false;
                                send_msg(client_fd, rsp);
                                free(rsp);
                                break;
                            }
                        } else {
                            send_msg(client_fd, "FAILED - Failed to send request");
                        }
                    }
                    last_activity = monotonic_time_sec();
                } else if (strncmp(req, "DO ", 3) == 0) {
                    // Forward DO command over TCP
                    if (send_msg(tcp_fd, req) == 0) {
                        char *rsp = NULL;
                        if (recv_msg(tcp_fd, &rsp, NULL) == 0 && rsp) {
                            send_msg(client_fd, rsp);
                            free(rsp);
                        } else {
                            send_msg(client_fd, "RSP FAILED - Router connection lost");
                        }
                    } else {
                        send_msg(client_fd, "RSP FAILED - Write error");
                    }
                    last_activity = monotonic_time_sec();
                } else if (strcmp(req, "STATUS") == 0) {
                    char buf[256];
                    uint64_t idle = now - last_activity;
                    snprintf(buf, sizeof(buf), "STATUS state=%s anchor=%d idle_sec=%lu granted=%s",
                             is_granted ? "GRANTED" : (request_pending ? "WAITING" : "UNGRANTED"),
                             (int)anchor, (unsigned long)idle, is_granted ? "yes" : "no");
                    send_msg(client_fd, buf);
                    last_activity = monotonic_time_sec();
                } else if (strcmp(req, "CANCEL") == 0) {
                    send_msg(tcp_fd, "CANCEL");
                    char *rsp = NULL;
                    if (recv_msg(tcp_fd, &rsp, NULL) == 0 && rsp) {
                        send_msg(client_fd, rsp);
                        free(rsp);
                    } else {
                        send_msg(client_fd, "RSP OK - Cancelled");
                    }
                    last_activity = monotonic_time_sec();
                } else if (strcmp(req, "CLOSE") == 0) {
                    send_msg(client_fd, "CLOSED");
                    send_msg(tcp_fd, "CLOSE");
                    free(req);
                    should_exit = true;
                    break;
                } else {
                    send_msg(client_fd, "ERROR Unknown IPC verb");
                }

                free(req);
            }
            close(client_fd);
            if (should_exit) {
                break;
            }
        }
    }

    close(tcp_fd);
    unlink(sock_path);
    close(listen_fd);
    exit(0);
}

static int ensure_daemon_running(pid_t anchor, const char *sock_path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        close(fd);
        return 0; // Already running
    }
    close(fd);

    // Stale socket cleanup
    unlink(sock_path);

    const char *endpoint = getenv("NETMON_LISTEN");
    if (!endpoint || !*endpoint) {
        endpoint = DEFAULT_ENDPOINT;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        run_daemon(anchor, endpoint, sock_path);
        _exit(0);
    }

    // Wait up to 250ms for daemon to bind socket
    for (int i = 0; i < 25; ++i) {
        usleep(10000); // 10ms
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                close(fd);
                return 0;
            }
            close(fd);
        }
    }

    return -1;
}

static int open_daemon_conn(pid_t anchor, const char *sock_path) {
    if (ensure_daemon_running(anchor, sock_path) != 0) {
        fprintf(stderr, "Error: Could not connect to or spawn clearance daemon\n");
        return -1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s <subcommand> [args...]\n\n"
            "Subcommands:\n"
            "  request <R|RW|RWP>   Request clearance approval for security level (R=read, RW=write, RWP=password)\n"
            "  do \"<zysh>\" [...]     Execute one or more quoted router commands sequentially\n"
            "  status               Query connection and clearance status\n"
            "  cancel               Cancel in-flight command\n"
            "  close                Close session and terminate background daemon\n",
            prog);
}

static const char *normalize_security_level(const char *arg) {
    if (!arg) return NULL;
    if (strcasecmp(arg, "R") == 0 || strcasecmp(arg, "read") == 0) {
        return "R";
    }
    if (strcasecmp(arg, "RW") == 0 || strcasecmp(arg, "write") == 0 || strcasecmp(arg, "read-write") == 0) {
        return "RW";
    }
    if (strcasecmp(arg, "RWP") == 0 || strcasecmp(arg, "password") == 0 || strcasecmp(arg, "elevated") == 0) {
        return "RWP";
    }
    return NULL;
}

static void build_request_msg(char *buf, size_t size, pid_t anchor, const char *level) {
    char host[64] = "unknown";
    if (gethostname(host, sizeof(host)) != 0) {
        strncpy(host, "unknown", sizeof(host) - 1);
    }
    host[sizeof(host) - 1] = '\0';

    const char *user = getenv("USER");
    if (!user || !*user) user = getenv("LOGNAME");
    if (!user || !*user) user = "unknown";

    const char *platform_env = getenv("AI_PLATFORM");
    const char *model_env = getenv("AI_AGENT_MODEL");
    const char *session_env = getenv("AI_CONVERSATION_ID");

    char platform_str[128];
    if (platform_env && *platform_env) {
        snprintf(platform_str, sizeof(platform_str), "%s (Linux %s, anchor PID %d, user %s)",
                 platform_env, host, (int)anchor, user);
    } else {
        snprintf(platform_str, sizeof(platform_str), "Antigravity IDE (Linux %s, anchor PID %d, user %s)",
                 host, (int)anchor, user);
    }

    const char *model = (model_env && *model_env) ? model_env : "Gemini-2.0";
    const char *session = (session_env && *session_env) ? session_env : "unknown";

    snprintf(buf, size, "REQUEST tier=\"%s\" platform=\"%s\" model=\"%s\" session=\"%s\"",
             level, platform_str, model, session);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *subcmd = argv[1];
    const char *normalized_level = NULL;

    if (strcmp(subcmd, "request") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'request' requires a security level: R, RW, or RWP\n");
            print_usage(argv[0]);
            return 1;
        }
        if (argc > 3) {
            fprintf(stderr, "Error: 'request' accepts no additional arguments after the security level\n");
            print_usage(argv[0]);
            return 1;
        }
        normalized_level = normalize_security_level(argv[2]);
        if (!normalized_level) {
            fprintf(stderr, "Error: Unknown security level '%s'. Must be R, RW, or RWP\n", argv[2]);
            print_usage(argv[0]);
            return 1;
        }
    } else if (strcmp(subcmd, "status") == 0 ||
               strcmp(subcmd, "cancel") == 0 ||
               strcmp(subcmd, "close") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Error: '%s' accepts no additional arguments\n", subcmd);
            print_usage(argv[0]);
            return 1;
        }
    } else if (strcmp(subcmd, "do") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: 'do' requires at least one command argument\n");
            print_usage(argv[0]);
            return 1;
        }
    } else {
        fprintf(stderr, "Error: Unknown subcommand '%s'\n", subcmd);
        print_usage(argv[0]);
        return 1;
    }

    pid_t anchor = get_anchor_pid();
    char sock_path[128];
    get_sock_path(sock_path, sizeof(sock_path), anchor);

    if (strcmp(subcmd, "request") == 0) {
        int fd = open_daemon_conn(anchor, sock_path);
        if (fd < 0) return 1;

        char req_msg[512];
        build_request_msg(req_msg, sizeof(req_msg), anchor, normalized_level);

        if (send_msg(fd, req_msg) != 0) {
            close(fd);
            return 1;
        }

        while (1) {
            char *rsp = NULL;
            if (recv_msg(fd, &rsp, NULL) != 0 || !rsp) {
                close(fd);
                return 1;
            }
            printf("%s\n", rsp);
            if (strncmp(rsp, "WAITING", 7) == 0) {
                free(rsp);
                continue;
            }
            int ok = (strncmp(rsp, "GRANTED", 7) == 0 || strncmp(rsp, "ALREADY_GRANTED", 15) == 0);
            free(rsp);
            close(fd);
            return ok ? 0 : 1;
        }
    } else if (strcmp(subcmd, "do") == 0) {
        int fd = open_daemon_conn(anchor, sock_path);
        if (fd < 0) return 1;

        // Verify session clearance status before sending commands
        send_msg(fd, "STATUS");
        char *status_rsp = NULL;
        if (recv_msg(fd, &status_rsp, NULL) == 0 && status_rsp) {
            bool granted = (strstr(status_rsp, "state=GRANTED") != NULL);
            if (!granted) {
                fprintf(stderr, "Error: Session has not been granted clearance. Run 'request <R|RW|RWP>' first.\n");
                free(status_rsp);
                close(fd);
                return 1;
            }
            free(status_rsp);
        }

        for (int i = 2; i < argc; ++i) {
            const char *zysh = argv[i];
            char req[MAX_REQ_LEN + 8];
            snprintf(req, sizeof(req), "DO %s", zysh);

            if (send_msg(fd, req) != 0) {
                close(fd);
                return 1;
            }

            char *rsp = NULL;
            if (recv_msg(fd, &rsp, NULL) != 0 || !rsp) {
                close(fd);
                return 1;
            }

            // Print response
            const char *body = rsp;
            if (strncmp(rsp, "RSP ", 4) == 0) {
                body = rsp + 4;
            }
            printf("%s\n", body);

            bool is_ok = (strncmp(body, "OK ", 3) == 0 || strcmp(body, "OK") == 0);
            free(rsp);

            if (!is_ok) {
                // Abort sequence immediately on non-OK response
                close(fd);
                return 1;
            }
        }
        close(fd);
        return 0;
    } else if (strcmp(subcmd, "status") == 0) {
        int fd = open_daemon_conn(anchor, sock_path);
        if (fd < 0) return 1;

        send_msg(fd, "STATUS");
        char *rsp = NULL;
        if (recv_msg(fd, &rsp, NULL) == 0 && rsp) {
            printf("%s\n", rsp);
            free(rsp);
            close(fd);
            return 0;
        }
        close(fd);
        return 1;
    } else if (strcmp(subcmd, "cancel") == 0) {
        int fd = open_daemon_conn(anchor, sock_path);
        if (fd < 0) return 1;

        send_msg(fd, "CANCEL");
        char *rsp = NULL;
        if (recv_msg(fd, &rsp, NULL) == 0 && rsp) {
            printf("%s\n", rsp);
            free(rsp);
            close(fd);
            return 0;
        }
        close(fd);
        return 1;
    } else if (strcmp(subcmd, "close") == 0) {
        int fd = open_daemon_conn(anchor, sock_path);
        if (fd < 0) {
            // Already closed/absent
            printf("CLOSED\n");
            return 0;
        }

        send_msg(fd, "CLOSE");
        char *rsp = NULL;
        if (recv_msg(fd, &rsp, NULL) == 0 && rsp) {
            printf("%s\n", rsp);
            free(rsp);
        } else {
            printf("CLOSED\n");
        }
        close(fd);
        return 0;
    }

    return 0;
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

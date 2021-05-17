#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xbase/xutl.h"
#include "xdata/xntp.h"
#include "xdata/xsysctl.h"
#include "xpbase/base/top_log.h"
#include "xdata/xoperation_config.h"

#if defined __MAC_PLATFORM__
#include <uuid/uuid.h>
#include <sys/param.h>
#include <sys/mount.h>
#elif defined __LINUX_PLATFORM__
#include <sys/statfs.h>
#include <grp.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace top {

/*
 * https://access.redhat.com/solutions/39194
 *
 * Shorter  polling intervals cause NTP to make large but less accurate calculations that never stabilize,
 * causing the local system clock to wander.
 * They are also useful if you want to make sure that your NTP daemon will detect an outage of the NTP peers in less time.
 *
 * Longer polling intervals allow NTP to calculate smaller tick adjustments
 * that stabilizes to a more accurate value, reducing wander in the local system  clock.
 *
 * 10 Mar 14:21:37 ntpdate[34164]: 45.76.244.202 rate limit response from server.
 * 10 Mar 14:22:42 ntpdate[34178]: 208.67.75.242 rate limit response from server.
 * 10 Mar 14:24:12 ntpdate[35889]: 208.67.75.242 rate limit response from server.
 * 10 Mar 14:24:17 ntpdate[35881]: 204.93.207.12 rate limit response from server.
 * 10 Mar 14:24:18 ntpdate[35919]: 198.60.22.240 rate limit response from server.
 * 10 Mar 14:24:21 ntpdate[35923]: 208.67.75.242 rate limit response from server.
 * 10 Mar 14:25:07 ntpdate[35937]: 208.67.75.242 rate limit response from server.
 * 10 Mar 14:25:20 ntpdate[35947]: 208.67.75.242 rate limit response from server.
 * 10 Mar 14:25:23 ntpdate[35959]: 50.101.251.61 rate limit response from server.
 * 10 Mar 14:25:33 ntpdate[35987]: 50.101.251.61 rate limit response from server.
 * 10 Mar 14:25:33 ntpdate[35981]: 198.60.22.240 rate limit response from server.
 * 10 Mar 14:25:36 ntpdate[35985]: 208.67.75.242 rate limit response from server.
 *
 *
 * https://forums.freebsd.org/threads/ntpdate-rate-limit-response-from-server.69556/
 *
 * If you're hitting a single server/pool from lots of machines inside your network: don't do that.
 * Set up your own NTP server(s) for your internal network.
 * This is usually always a good idea for any network bigger than ~5 machines to keep all of them in sync even while your uplink is down.
 */

int32_t ntp_sync(const std::vector<std::string>& ntp_server_list) {
    if (ntp_server_list.empty()) {
        return -1;
    }

    int itr = 0;
    int ntp_cmd_successed = -1;

    // perform an NTP sync with ntp server
    for (auto& ntp_host : ntp_server_list) {
        xkinfo("current ntp server: %s", ntp_host.c_str());
#if defined (__MAC_PLATFORM__)
#ifdef DEBUG
        const std::string cmd = "sntp -sS " + ntp_host + " >> /tmp/ntp.log 2>&1";
#else
        const std::string cmd = "sntp -sS " + ntp_host + " > /dev/null 2>&1";
#endif
        ntp_cmd_successed = top::base::xsys_utl::system_cmd(cmd.c_str());
#elif defined (__LINUX_PLATFORM__)
#ifdef DEBUG
        const std::string cmd = "ntpdate -u " + ntp_host + " >> /tmp/ntp.log 2>&1";
#else
        const std::string cmd = "ntpdate -u " + ntp_host + " > /dev/null 2>&1";
#endif
        ntp_cmd_successed = top::base::xsys_utl::system_cmd(cmd.c_str());
#endif
        // when command execution failed, fallback on the mannual way
        if (ntp_cmd_successed != 0) {
            int32_t xit_err = -1;
            uint64_t xut_timev = 0ULL;

#ifdef _WIN32
            vxWSASocketInit gInit;
#endif // _WIN32

            xut_timev = 0ULL;
            xit_err = top::ntp_get_time(ntp_host.c_str(), NTP_PORT, 5000, &xut_timev);
            if (0 == xit_err) {
                // successfully synced with current ntp server
                top::bv_output(ntp_host.c_str(), xut_timev);
                if (top::ntp_settimeofday(xut_timev) == 0) {
                    xkinfo("system time has been synced successfully");
                } else {
                    xkinfo("settimeofday failed, detail(%d:%s)", errno, strerror(errno));
                }
                break;
            } else {
                xkinfo("%s return error code : %d\n", ntp_host.c_str(), xit_err);
                ++itr;
            }

            // too many tries this time, maybe the network error, try it next timer
            if (itr >= 5) {
                break;
            }
        } else {
            // successfully synced with current ntp server
            break;
        }
    }
    return 0;
}

static int check_log_file(const std::string& full_log_file_path) {
    // check file's last modified time
    struct stat file_stat;
    if (stat(full_log_file_path.c_str(), &file_stat) != 0) {
        xkinfo("stat failed, path(%s), detail(%d:%s)", full_log_file_path.c_str(), errno, strerror(errno));
        return -1;
    }

    struct timeval now;
    gettimeofday(&now, nullptr);

    // remove file 6 hours before current time
#if defined (__MAC_PLATFORM__)
    int hour = (now.tv_sec - file_stat.st_mtimespec.tv_sec) / 3600;
#elif defined (__LINUX_PLATFORM__)
    int hour = (now.tv_sec - file_stat.st_mtime) / 3600;
#else
    #error "unsupported platform"
#endif
    xkinfo("time diff in hour(%d) for file(%s)", hour, full_log_file_path.c_str());
    if (hour >= 6) {
        xkinfo("remove log file path(%s)", full_log_file_path.c_str());
        if (remove(full_log_file_path.c_str()) != 0) {
            xkinfo("remove failed, path(%s), detail(%d:%s)", full_log_file_path.c_str(), errno, strerror(errno));
            return -1;
        }
        return 0;
    }
    return -1;
}

void check_log_path(const std::string& log_path) {
    DIR *dirp = opendir(log_path.c_str());
    if (dirp == nullptr) {
        xkinfo("opendir failed, path(%s), detail(%d:%s)", log_path.c_str(), errno, strerror(errno));
        return;
    } else {
        struct dirent *entry;
        while ((entry = readdir(dirp)) != nullptr) {
            // skip current directory and parent directory to avoid recurrence.
            if (strncmp(entry->d_name, ".", 1) == 0 || strncmp(entry->d_name, "..", 2) == 0) {
                continue;
            }

            if (DT_REG == entry->d_type || DT_UNKNOWN == entry->d_type) {
                // check log file's last modified time, sample: "xtop.2019-07-04-144108-1-17497.log"
                if (strncmp(entry->d_name, "xtop", 4) == 0 && strstr(entry->d_name, ".log") && strlen(entry->d_name) > sizeof("xtop.log")) {
                    std::string full_file_path = log_path + "/" + entry->d_name;
                    check_log_file(full_file_path);
                }
            }
        }

        closedir(dirp);
    }
}

int daemonize() {
    int fd;

    switch (fork()) {
    case -1:
        xerror("fork() failed, details(%d:%s)", errno, strerror(errno));
        return -1;

    case 0:
        break;

    default:
        exit(0);
    }

    // xtop_pid = getpid();

    if (setsid() == -1) {
        xerror("setsid() failed, details(%d:%s)", errno, strerror(errno));
        return -1;
    }

    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        xkinfo("open(\"/dev/null\") failed, details(%d:%s)", errno, strerror(errno));
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        xkinfo("dup2(STDIN) failed, details(%d:%s)", errno, strerror(errno));
        return -1;
    }

#if 0
    if (dup2(fd, STDOUT_FILENO) == -1) {
        xkinfo("dup2(STDOUT) failed, details(%d:%s)", errno, strerror(errno));
        return -1;
    }

    if (dup2(fd, STDERR_FILENO) == -1) {
        xkinfo("dup2(STDERR) failed, details(%d:%s)", errno, strerror(errno));
        return -1;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            xkinfo("close()) failed, details(%d:%s)\n", errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int drop_root(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (pw == NULL) {
        xwarn("getpwnam failed, username(%s), detail(%d:%s)\n", username, errno, strerror(errno));
        printf("getpwnam failed, username(%s), detail(%d:%s)\n", username, errno, strerror(errno));
        return -1;
    }

    if (initgroups(pw->pw_name, pw->pw_gid) != 0 ||
        setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0) {
        xkinfo("Couldn't change to '%.32s' uid=%lu gid=%lu: details(%d:%s)\n",
               username,
               (unsigned long)pw->pw_uid,
               (unsigned long)pw->pw_gid,
               errno,
               strerror(errno));

        printf("Couldn't change to '%.32s' uid=%lu gid=%lu: details(%d:%s)\n",
               username,
               (unsigned long)pw->pw_uid,
               (unsigned long)pw->pw_gid,
               errno,
               strerror(errno));

        return -1;
    }
#ifdef DEBUG
    printf("drop_root ok\n");
#endif
    return 0;
}

void get_groups() {
    const int gidsetsize = NGROUPS_MAX;
    gid_t grouplist[NGROUPS_MAX];

    int n = getgroups(gidsetsize, grouplist);
    if (n == -1) {
        xkinfo("getgroups() failed, details(%d:%s)", errno, strerror(errno));
        return;
    }

    std::stringstream ss;
    ss << "grouplist: ";
    for (int i = 0; i < n; i++) {
        ss << grouplist[i] << " ";
    }

    xkinfo("%s", ss.str().c_str());
}

/*
 *       soft  nofile   262144
 *       hard  nofile   262144
 *       soft  core     524288000
 *       soft  sigpending 65535
 *       hard  sigpending 65535
 */

int set_limits() {
    save_limits_conf();

    // set for current session
    pid_t sys_pid = getpid();
    struct rlimit rl_now;

    if(0 == getrlimit(RLIMIT_NOFILE, &rl_now)) {
#ifdef __MAC_PLATFORM__
        rl_now.rlim_cur = std::min((uint64_t)OPEN_MAX, rl_now.rlim_max);
        if(0 != setrlimit(RLIMIT_NOFILE, &rl_now)) {
            fprintf(stderr, "xnode(%d), setrlimit(RLIMIT_NOFILE), details(%d:%s)\n", sys_pid, errno, strerror(errno));
            xwarn("xnode(%d), setrlimit(RLIMIT_NOFILE), details(%d:%s)", sys_pid, errno, strerror(errno));
            return -1;
        }
#else //else of __MAC_PLATFORM__
        if(rl_now.rlim_cur < 65535) { //at least need give 65535
            rl_now.rlim_cur = 65535;
            rl_now.rlim_max = std::max((uint64_t)65535, rl_now.rlim_max);
            if(0 != setrlimit(RLIMIT_NOFILE, &rl_now)) {
                fprintf(stderr, "xnode(%d), setrlimit(RLIMIT_NOFILE), details(%d:%s)\n", sys_pid, errno, strerror(errno));
                xwarn("xnode(%d), setrlimit(RLIMIT_NOFILE), details(%d:%s)", sys_pid, errno, strerror(errno));
                return -1;
            }
        }
#endif
    } else {
        fprintf(stderr, "xnode(%d), getrlimit(RLIMIT_NOFILE), details(%d:%s)\n", sys_pid, errno, strerror(errno));
        xwarn("xnode(%d), getrlimit(RLIMIT_NOFILE), details(%d:%s)\n", sys_pid, errno, strerror(errno));
        return -1;
    }

    if(0 == getrlimit(RLIMIT_CORE, &rl_now)) {
#ifndef NDEBUG
        rl_now.rlim_cur = 524288000;
#else
        rl_now.rlim_cur = 0;  // no core files under release mode
#endif
        if(0 != setrlimit(RLIMIT_CORE, &rl_now)) {
            fprintf(stderr, "xnode(%d), setrlimit(RLIMIT_CORE), details(%d:%s)\n", sys_pid, errno, strerror(errno));
            xwarn("xnode(%d), setrlimit(RLIMIT_CORE), details(%d:%s)", sys_pid, errno, strerror(errno));
            return -1;
        }
    } else {
        fprintf(stderr, "xnode(%d), getrlimit(RLIMIT_CORE), details(%d:%s)\n", sys_pid, errno, strerror(errno));
        xwarn("xnode(%d), getrlimit(RLIMIT_CORE), details(%d:%s)", sys_pid, errno, strerror(errno));
        return -1;
    }

#if defined (__LINUX_PLATFORM__)
    if(0 == getrlimit(RLIMIT_SIGPENDING, &rl_now)) {
        if(rl_now.rlim_cur < 65535) { //at least need give 65535
            rl_now.rlim_cur = 65535;
            rl_now.rlim_max = std::max((uint64_t)65535, rl_now.rlim_max);
            if(0 != setrlimit(RLIMIT_NOFILE, &rl_now)) {
                fprintf(stderr, "xnode(%d), setrlimit(RLIMIT_SIGPENDING), details(%d:%s)\n", sys_pid, errno, strerror(errno));
                xwarn("xnode(%d), setrlimit(RLIMIT_SIGPENDING), details(%d:%s)", sys_pid, errno, strerror(errno));
                return -1;
            }
        }
    } else {
        fprintf(stderr, "xnode(%d), getrlimit(RLIMIT_SIGPENDING), details(%d:%s)\n", sys_pid, errno, strerror(errno));
        xwarn("xnode(%d), getrlimit(RLIMIT_SIGPENDING), details(%d:%s)", sys_pid, errno, strerror(errno));
        return -1;
    }
#endif

    const int total_cpu_cores = top::base::xsys_utl::sys_cpu_cores();
    xkinfo("System total cpu cores:%d, and kernel:%s", total_cpu_cores, top::base::xsys_utl::kernel_version().c_str());
    getrlimit(RLIMIT_NOFILE, &rl_now);
    xkinfo("getrlimit(RLIMIT_NOFILE) is soft(%lld) and hard(%lld)", rl_now.rlim_cur, rl_now.rlim_max);

    return 0;
}

int get_disk_space(const std::string& compoment_path) {
    struct statfs disk_info;

    if (statfs(compoment_path.c_str(), &disk_info)) {
        xkinfo("statfs(%s) failed, details(%d:%s)", compoment_path.c_str(), errno, strerror(errno));
        return -1;
    }

    unsigned long long block_size = disk_info.f_bsize;
    unsigned long long total_size = block_size * disk_info.f_blocks;
    xkinfo("path(%s) total size = %llu GB\n", compoment_path.c_str(), total_size >> 30);

    unsigned long long free_disk = disk_info.f_bfree * block_size;
    unsigned long long available_disk = disk_info.f_bavail * block_size;
    xkinfo("path(%s) disk free = %llu GB, disk available = %llu GB",
                    compoment_path.c_str(), free_disk >> 30, available_disk >> 30);
    return 0;
}

}

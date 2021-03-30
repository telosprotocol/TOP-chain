#include <string>
#include <fstream>
#include "xbase/xutl.h"
#include "xdata/xsysctl.h"
#include "xdata/sysctl.conf.h"
#include "xdata/limits.conf.h"
#include "xpbase/base/top_log.h"

namespace top
{

void save_sysctl_conf() {
#if defined (__LINUX_PLATFORM__)
    const std::string conf = sysctl_conf;
    const std::string path = "/etc/sysctl.conf";
    std::ofstream ofs;
    ofs.open(path.c_str());
    if (!ofs.good()) {
        TOP_FATAL("write %s failed", path.c_str());
        return;
    }
    ofs << conf;
    ofs.close();
#endif
}

int set_sysctl()
{
    save_sysctl_conf();

    // take effect
#ifdef DEBUG
    std::string sysctl_cmd = "/sbin/sysctl -p";
#else
    std::string sysctl_cmd = "/sbin/sysctl -p > /dev/null 2>&1";
#endif
    return top::base::xsys_utl::system_cmd(sysctl_cmd.c_str());
}

void save_limits_conf() {
#if defined (__LINUX_PLATFORM__)
    const std::string conf = limits_conf;
    const std::string path = "/etc/security/limits.conf";
    std::ofstream ofs;
    ofs.open(path.c_str());
    if (!ofs.good()) {
        TOP_FATAL("write %s failed", path.c_str());
        return;
    }
    ofs << conf;
    ofs.close();
    // need relogin
#endif
}

}

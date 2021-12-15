// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include <sys/time.h>
#if defined(__LINUX_PLATFORM__)
#include <sys/prctl.h>
#endif
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <chrono>
#include <limits>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "xmonitor.h"
#include "simplewebserver/client_http.hpp"
#include "generated/version.h"
#include "topio_setproctitle.h"
#include "safebox_http.h"
#include "CLI11.hpp"
#include "xnode/xconfig.h"

 // nlohmann_json
 #include <nlohmann/json.hpp>
 using json = nlohmann::json;

using namespace top;
////////////////////////define configure file path and name/////////////////////////
char** topio_os_argv;
int topio_os_argc;
static const std::string TOPIO_HOME_ENV = "TOPIO_HOME";
static const uint32_t MAX_PASSWORD_SIZE = 1024;
static std::atomic<bool> child_reap{false};
static std::atomic<uint32_t> child_exit_code {0};
static pid_t child_pid = -1;
static std::string pid_file;
static std::string safebox_pid;
static std::vector<std::string> support_cmd_vec = {
    "help", "version", "node", "mining", "staking",
    "chain","transfer", "govern", "resource", "wallet", "db", "debug"};


xnode_signal_t  multi_process_signals[] = {
    { SIGCHLD, "SIGCHLD", "", xnode_signal_handler },
    { SIGHUP, "SIGHUP", "", xnode_signal_handler },
    { SIGTERM, "SIGTERM", "", xnode_signal_handler },
    { SIGINT, "SIGINT", "", xnode_signal_handler },
    { 0, NULL, "", NULL }
};

xnode_signal_t  normal_signals[] = {
    { SIGTERM, "SIGTERM", "", xnode_signal_handler },
    { SIGINT, "SIGINT", "", xnode_signal_handler },
    { 0, NULL, "", NULL }
};

static int check_log_file(const std::string& full_log_file_path) {
    // check file's last modified time
    struct stat file_stat;
    if (stat(full_log_file_path.c_str(), &file_stat) != 0) {
        fprintf(stderr, "stat failed, path(%s), detail(%d:%s)", full_log_file_path.c_str(), errno, strerror(errno));
        return -1;
    }

    struct timeval now;
    gettimeofday(&now, nullptr);

    // remove file 6 hours before current time
    int hour = (now.tv_sec - file_stat.st_mtime) / 3600;
    fprintf(stdout, "time diff in hour(%d) for file(%s)", hour, full_log_file_path.c_str());
    if (hour >= 6) {
        fprintf(stdout, "remove log file path(%s)", full_log_file_path.c_str());
        if (remove(full_log_file_path.c_str()) != 0) {
            fprintf(stderr, "remove failed, path(%s), detail(%d:%s)", full_log_file_path.c_str(), errno, strerror(errno));
            return -1;
        }
        return 0;
    }
    return -1;
}

void check_log_path(const std::string& log_path) {
    DIR *dirp = opendir(log_path.c_str());
    if (dirp == nullptr) {
        fprintf(stderr, "opendir failed, path(%s), detail(%d:%s)", log_path.c_str(), errno, strerror(errno));
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

// load dynamic lib libxtopchain.so
int load_lib(config_t& config) {
    const std::string target_component_solib = "lib" + config.target_component_name + ".so";  // libxtopchain.so

    void* component_handle = dlopen(target_component_solib.c_str(), RTLD_LAZY);
    if (NULL == component_handle) {
        std::cerr << "topio not found component:" << target_component_solib << std::endl;
        return -1;
    }

    int init_res = -1;
    if (config.so_func_name == "get_version") {
        typedef int (*init_component_function_t)();
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }

        init_res = (*init_function_ptr)();
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    } else if (config.so_func_name == "init_component") {
        typedef int (*init_component_function_t)(const char*, const char*);
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
        std::string param1 = config.config_file;
        std::string param2 = config.config_file_extra;
        // run node, will block here
        init_res = (*init_function_ptr)(param1.c_str(), param2.c_str());
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    } else if (config.so_func_name == "init_noparams_component") {
        typedef int (*init_component_function_t)(const char*,const uint16_t,const char*,const uint16_t,const char*, const char*, const char*);
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
        // run node, will block here
        init_res = (*init_function_ptr)(
                config.pub_key.c_str(),
                config.pub_key.size(),
                config.pri_key.c_str(),
                config.pri_key.size(),
                config.node_id.c_str(),
                config.datadir.c_str(),
                config.config_file_extra.c_str());
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    } else if (config.so_func_name == "parse_execute_command") {
        typedef int (*init_component_function_t)(const char*, int, char**);
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
        init_res = (*init_function_ptr)(config.config_file_extra.c_str(), config.argc, config.argv);
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    } else if (config.so_func_name == "decrypt_keystore") {
        typedef int (*init_component_function_t)(const char*, const char*, uint8_t*, uint32_t&);
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
        uint8_t prikey[4096];
        bzero(prikey, sizeof(prikey));
        uint32_t prikey_size;
        init_res = (*init_function_ptr)(config.keystore_path.c_str(), config.password.c_str(), prikey, prikey_size);
        //fprintf(stdout, "prikeysize:%u\n", prikey_size);
        config.pri_key = std::string((char*)prikey, prikey_size);
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    } else if (config.so_func_name == "decrypt_keystore_by_key") {
        typedef int (*init_component_function_t)(const char*, const char*, uint8_t*, uint32_t&);
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
        uint8_t prikey[4096];
        bzero(prikey, sizeof(prikey));
        uint32_t prikey_size;
        init_res = (*init_function_ptr)(config.keystore_path.c_str(), config.token.c_str(), prikey, prikey_size);
        //fprintf(stdout, "prikeysize:%u\n", prikey_size);
        config.pri_key = std::string((char*)prikey, prikey_size);
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    } else if (config.so_func_name == "check_miner_info") {
        typedef int (*init_component_function_t)(const char*, uint16_t, const char*);
        init_component_function_t init_function_ptr = nullptr;
        init_function_ptr = (init_component_function_t)dlsym(component_handle, config.so_func_name.c_str());
        if (init_function_ptr == nullptr) {
            //fprintf(stderr, "topio found no function named:%s in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
        // run node, will block here
        init_res = (*init_function_ptr)(
                config.pub_key.c_str(),
                config.pub_key.size(),
                config.node_id.c_str());
        if (init_res != 0) {
            //fprintf(stderr,"topio load function named:%s fail in lib:%s\n", config.so_func_name.c_str(), target_component_solib.c_str());
            dlclose(component_handle);
            return -1;
        }
    }


    dlclose(component_handle);
    return init_res;
}

int init_log() {
    return 0;
}

//use to check if a file exists
bool isFileExist (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

//use to check if a directory exists
bool isDirExist(std::string dirPath){
    struct stat fileInfo;
    stat(dirPath.c_str(), &fileInfo);
    if (S_ISDIR(fileInfo.st_mode)){
        return true;
    }else{
        return false;
    }
}

bool scan_keystore_dir(
        const std::string& path,
        std::vector<std::map<std::string, std::string>>& keystore_vec) {
    std::vector<std::string> keystore_file;
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr){
        std::cout << "open directory:" << path << " failed" << std::endl;
        return false;
    }
    struct dirent * filename;
    while((filename = readdir(dir)) != nullptr)
    {
        if(strcmp(filename->d_name, ".") == 0
                || strcmp(filename->d_name, "..") == 0) {
           continue;
        }
        std::string keyfile_path = path + "/" + std::string(filename->d_name);
        keystore_file.push_back(keyfile_path);
    }

    for (const auto& kfile : keystore_file) {
        std::ifstream keyfile(kfile);
        if(!keyfile.is_open()){
            continue;
        }

        json reader;
        try {
            keyfile >> reader;
        } catch (json::parse_error& e) {
            keyfile.close();
            continue;
        }
        keyfile.close();

        std::map<std::string, std::string> key_info_map = {
            {"account",    ""},
            {"public_key", ""},
            {"path",       ""},
            {"key_type",   ""}
        };

        for (const auto& el : reader.items()) {
            if (el.key() == "account address" || el.key() == "account_address") {
                key_info_map["account"] = el.value().get<std::string>();
            }
            if (el.key() == "public_key") {
                key_info_map["public_key"] = el.value().get<std::string>();
            }
            if (el.key() == "key_type") {
                key_info_map["key_type"] = el.value().get<std::string>();
            }
        }
        if (key_info_map.at("account").empty()
                || key_info_map.at("public_key").empty()
                || key_info_map.at("key_type").empty()) {
            std::cout << "ignore invalid keystore:" << kfile << std::endl;
            continue;
        }
        key_info_map["path"] = kfile;

        keystore_vec.push_back(key_info_map);
    }
    if (keystore_vec.empty()) {
        return false;
    }
    return true;
}

bool process_version_operation(config_t& config) {
    load_lib(config);
    return true;
}

// start daemon
bool daemon() {
    int fd;

    switch (fork()) {
        case -1:
            return false;
        case 0:
            // the first child process
            break;
        default:
            // exit if father process
            exit(0);
    }

    // setsid() will create a new session and then, the first child will detach from tty and process group,
    // and become the leader of this session. Right now the first child became the leader of no-tty session,
    // but this child process still has the ability to open  a tty
    if (setsid() == -1 ) {
        // setsid error
        return false;
    }

    /*
    // the second fork in order to make the first child process non-leader, avoid the ability to open a tty
    switch (fork()) {
        case -1:
            return false;
        case 0:
            // the second process
            break;
        default:
            // exit the first child
            exit(0);
    }
    */

    // reset umask
    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        return false;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        return false;
    }

#if 0
    if (dup2(fd, STDERR_FILENO) == -1) {
        return;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            return false;
        }
    }

    return true;
}

int spawn_child(config_t& config) {
    pid_t pid = fork();
    switch (pid) {
        case -1: {
            fprintf(stderr, "fork() failed while spawning, details(%d:%s)", errno, strerror(errno));
            return -1;
        }
        case 0: {
            //prctl(PR_SET_NAME, "xnode", NULL, NULL, NULL);
            // this is child

            const char *new_process_name = "xnode process";
            std::string new_title;
            for (int i = 0; i < topio_os_argc && topio_os_argv[i]; ++i) {
                if (i == 0) {
                    new_title += new_process_name;
                    new_title += " ";
                } else {
                    new_title += topio_os_argv[i];
                    new_title += " ";
                }
            }
            if (new_title.back() == ' ') {
                new_title.pop_back();
            }

            // set process title
            if (topio_init_setproctitle() == 0) {
                topio_setproctitle(new_title.c_str());
            }
            return load_lib(config);
        }
        default: {
            child_pid = pid;

            const char *new_process_name = "daemon process";
            std::string new_title;
            for (int i = 0; i < topio_os_argc && topio_os_argv[i]; ++i) {
                if (i == 0) {
                    new_title += new_process_name;
                    new_title += " ";
                } else {
                    new_title += topio_os_argv[i];
                    new_title += " ";
                }
            }
            if (new_title.back() == ' ') {
                new_title.pop_back();
            }

            // set process title
            if (topio_init_setproctitle() == 0) {
                topio_setproctitle(new_title.c_str());
            }

            break;
        }
    } // end switch(pid)

    return 0;
}

int start_worker_monitor_thread(config_t& config) {
    auto thread_proc = std::bind(CheckReStartXtopchain, config);
    std::thread(thread_proc).detach();

    return 0;
}

int start_monitor_thread(config_t& config) {
#ifndef DEBUG
    // only enable in debug mode
    auto log_thread_proc = std::bind(log_monitor_proc, config);
    std::thread(log_thread_proc).detach();
#endif

    auto cpu_thread_proc = std::bind(cpu_monitor_proc, config);
    std::thread(cpu_thread_proc).detach();

    return 0;
}

int cpu_monitor_proc(config_t config) {
    xmonitor_t monitor;
    for (;;) {
        monitor.monitor_cpu_and_network();
        std::this_thread::sleep_for(std::chrono::seconds(config.cpu_net_interval));
    }

    return 0;
}

int log_monitor_proc(config_t config) {
    while(true) {
        check_log_path(config.datadir + "/log");
        std::this_thread::sleep_for(std::chrono::seconds(10 * 60)); // every 10 min
    }
    return 0;
}

void signal_handler(int signo) {
    // avoid malloc dead lock(do not using malloc here, do not use LOG module)
    if (signo == SIGCHLD) {
        get_child_status();
    } else if (signo == SIGHUP) {  // reload, kill + child_reap
#ifdef DEBUG
        std::cout << "catch SIGHUP" << std::endl;
#endif
        if (kill(child_pid, SIGKILL) == -1) {
            std::cout << "kill() failed:" << strerror(errno) << "," << child_pid << "," << getpid() << std::endl;
        }
    } else if (signo == SIGTERM || signo == SIGINT) {
#ifdef DEBUG
        std::cout << "catch SIGTERM or SIGINT, will exit" << std::endl;
#endif
        if (!pid_file.empty()) {
            std::ifstream in_pid(pid_file);
            if (in_pid.is_open()) {
                char buff[16];
                bzero(buff, sizeof(buff));
                in_pid.getline(buff, 16);
                uint32_t xnode_pid = std::stoi(std::string(buff));
                auto runing_xnode_pid = static_cast<uint32_t>(getpid());
                if (xnode_pid == runing_xnode_pid) {
                    remove(pid_file.c_str());
                }
            }
        } else if (!safebox_pid.empty()){
            std::ifstream in_pid(safebox_pid);
            if (in_pid.is_open()) {
                char buff[16];
                bzero(buff, sizeof(buff));
                in_pid.getline(buff, 16);
                uint32_t pid = std::stoi(std::string(buff));
                auto runing_safebox_pid = static_cast<uint32_t>(getpid());
                if (pid == runing_safebox_pid) {
                    remove(safebox_pid.c_str());
                }
            }
        }


        ::_Exit(0);
    }
}

bool check_process_running(const std::string &pid_file) {
    if (pid_file.empty()) {
        return false;
    }

    std::ifstream in_pid(pid_file);
    if (!in_pid.is_open()) {
        return false;
    }

    char buff[16];
    bzero(buff, sizeof(buff));
    in_pid.getline(buff, 16);
    in_pid.close();
    uint32_t topio_pid = std::stoi(std::string(buff));

    // kill 0 test process running or not
    if (kill(topio_pid, 0) == -1) {
        // kill 0 error, process not exist
        return false;
    }

    return true;
}

void xnode_signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
    signal_handler(signo);
}


int register_signals(bool multi_process) {
    xnode_signal_t     *sig;
    if (multi_process) {
        sig = multi_process_signals;
    } else {
        sig = normal_signals;
    }
    struct sigaction   sa;

    for (; sig->signo != 0; sig++) {
        bzero(&sa, sizeof(struct sigaction));

        if (sig->handler) {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags = SA_SIGINFO;

        } else {
            sa.sa_handler = SIG_IGN;
        }

        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1) {
            std::cout << "sigaction(" << sig->signame << ") failed" << std::endl;
        }
    }

    return 0;
}

void get_child_status() {
    int status = 0;
    pid_t pid = -1;
    int err = 0;
    int one = 0;

    for (;;) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) {
            return;
        }

        if (pid == -1) {
            err = errno;
            if (err == EINTR) {
                continue;
            }
            if (err == ECHILD && one) {
                return;
            }
            if (err == ECHILD) {
                return;
            }
            return;
        }

        // ntpupdate also notify SIGCHLD
        if (pid == child_pid) {
            one = 1;
            child_reap = true;
            std::cout << "child reap" << std::endl;
            // check if killed by sig or by self exit
            if (WTERMSIG(status)) {
                std::cout << "exited with signal:" << WTERMSIG(status) << std::endl;
            } else {
                child_exit_code = WEXITSTATUS(status);
                std::cout << "exited with code:" << WEXITSTATUS(status) << std::endl;
            }
        }

    } // end for(;;)
}


void CheckReStartXtopchain(config_t& config) {
    if (!config.multiple_process) {
        return;
    }
    while (true) {
        if (child_reap) {
            child_reap = false;
            // load child process auto
            int wait_max = 8;
            for (int i = 0; i < wait_max; ++i) {
                std::cout << "worker stopped, wait to restart... " << wait_max - i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            spawn_child(config);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return;
}

// generate extra config or update
bool generate_extra_config(config_t& config) {
    json jextra;
    std::string extra_config = config.datadir + "/.extra_conf.json";
    do {
        if (!isFileExist(extra_config)) {
            break;
        }

        // update config
        std::ifstream in(extra_config);
        if(!in.is_open()){
            std::cerr << "can't open file: " << extra_config << std::endl;
            break;
        }

        json reader;
        try {
            in >> reader;
        } catch (json::parse_error& e) {
            in.close();
            std::cout << "parse config file:" << extra_config << " failed" << std::endl;
            break;
        }
        jextra  = reader;
    } while (0);

    // create config or update config
    jextra["admin_http_addr"] = config.admin_http_addr;
    jextra["admin_http_port"] = config.admin_http_port;
    // other key:values ,such as bootstrap ip
    jextra["net_port"]        = config.net_port;
    jextra["bootnodes"]       = config.bootnodes;
    jextra["datadir"]         = config.datadir;


    // write prettified JSON to another file
    std::ofstream out(extra_config);
    out << std::setw(4) << jextra << std::endl;
    out.close();
    config.config_file_extra = extra_config;
    return true;
}

bool get_default_miner(config_t& config,std::map<std::string, std::string> &default_miner) {
    std::string extra_config = config.datadir + "/.extra_conf.json";

    std::ifstream in(extra_config);
    if(!in.is_open()){
        std::cerr << "can't open file: " << extra_config << std::endl;
        return false;
    }
    json reader;
    try {
        in >> reader;
        default_miner["path"]        = reader["default_miner_keystore"].get<std::string>();
        if (!isFileExist(default_miner.at("path"))) {
            return false;
        }

        default_miner["node_id"]     = reader["default_miner_node_id"].get<std::string>();
        default_miner["public_key"]  = reader["default_miner_public_key"].get<std::string>();
        //default_miner["private_key"] = reader["default_miner_private_key"].get<std::string>();
    } catch (...) {
        in.close();
        return false;
    }
    in.close();

    // get miner private key from mem(safebox)
    auto prikey = get_prikey_from_safebox(reader["default_miner_public_key"].get<std::string>());
    if (prikey.empty()) {
        return false;
    }
    // for safety, prikey from safebox not final privatekey, it's token
    //default_miner["private_key"] = prikey;
    default_miner["token"] = prikey;

    if (default_miner.at("token").empty()) {
        return false;
    }

    return true;
}

std::string get_prikey_from_safebox(const std::string &account) {
    using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
    std::string daemon_host = "127.0.0.1:7000";
    HttpClient client(daemon_host);
    json body = json::object();
    body["method"] = "get";
    body["account"] = account;
    std::string token_request = body.dump();
    std::string token_response_str;
    try {
        SimpleWeb::CaseInsensitiveMultimap header;
        header.insert({"Content-Type", "application/json"});
        auto token_response = client.request("POST", "/api/safebox", token_request, header);
        token_response_str = token_response->content.string();
    } catch (std::exception & e) {
        std::cout << "catch exception:" << e.what() << std::endl;
        return "";
    }

    json response;
    try {
        response = json::parse(token_response_str);
    } catch (json::parse_error& e) {
        std::cout << "json parse failed" << std::endl;
        return "";
    }
    auto jfind = response.find("private_key");
    if (jfind == response.end()) {
        return "";
    }
    return response["private_key"].get<std::string>();
}


void block_loop() {
    while (true) {
        // sleep forerver
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

// master-worker mode
void start_master_worker_mode(config_t& config) {
    if (register_signals(true) != 0) {
        std::cerr << "register signals failed" << std::endl;
        return;
    }

    spawn_child(config);
    start_monitor_thread(config);
    start_worker_monitor_thread(config);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    // will block here
    block_loop();
}

// single process mode
void start_single_process_mode(config_t& config) {
    if (register_signals(false) != 0) {
        std::cerr << "register signals failed" << std::endl;
        return;
    }
    auto thread_proc = std::bind(load_lib, config);
    std::thread(thread_proc).detach();

    start_monitor_thread(config); // will detach
    // do not need check worker status for single process
    //start_worker_monitor_thread(config);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    // will block here
    block_loop();
}


// rewrite xtopchain json config file, using base dir datadir
bool update_component_config_file(config_t& config) {
    return true;

#ifdef NDEBUG
    // only for debug mode
    return true;
#endif
    auto config_file = config.config_file;
    if (config_file.empty()) {
        // maybe start with no params
        return true;
    }
    std::ifstream in(config_file);
    if(!in.is_open()){
        std::cerr << "can't open file: " << config_file << std::endl;
        assert(false);
        return false;

    }
    json reader;
    try {
        in >> reader;
    } catch (json::parse_error& e) {
        in.close();
        std::cout << "parse config file:" << config_file << " failed" << std::endl;
        return false;
    }
    in.close();

    for (auto &el : reader.items()) {
        if (el.key() == "db_path") {
            reader[el.key()] = config.datadir + "/db";
        } else if (el.key() == "log_path") {
            reader[el.key()] = config.datadir + "/log";
            config.com_log_path = config.datadir + "/log";
        }
    }

    reader["datadir"] = config.datadir;

    // write prettified JSON to another file
    std::ofstream out(config.config_file);
    out << std::setw(4) << reader << std::endl;
    out.close();
    return true;
}

bool setminerkey(config_t& config) {
    if (config.datadir.empty()) {
        std::cout << "error:datadir empty" << std::endl;
        return false;
    }
    if (config.pub_key.empty()) {
        std::cout << "publickey is required to setminerkey" << std::endl;
        return false;
    }

    auto keystore_dir = config.datadir + "/keystore";
    std::vector<std::map<std::string, std::string>> keystore_vec;
    scan_keystore_dir(keystore_dir, keystore_vec);
    for (const auto& v : keystore_vec) {
        if (v.at("public_key") == config.pub_key) {
            config.keystore_path = v.at("path");
            break;
        }
    }
    if (config.keystore_path.empty()) {
        std::cout << "The key does not exist in wallet." << std::endl;
        return false;
    }
    // keystore_path and password

    config.so_func_name = "decrypt_keystore";
    int libret = load_lib(config);
    if (libret != 0) {
        std::cout << "decrypt keystore:" << config.keystore_path << " failed" << std::endl;
        return false;
    }
    return true;
}

bool load_keystore(config_t& config) {
    if (config.datadir.empty()) {
        std::cout << "error:datadir empty" << std::endl;
        return false;
    }

    std::map<std::string, std::string> default_miner;
    if (get_default_miner(config, default_miner)) {
        config.node_id = default_miner.at("node_id");
        config.pub_key = default_miner.at("public_key");
        config.keystore_path = default_miner.at("path");
        //config.pri_key = default_miner.at("private_key");
        //return true;

        // get final private_key from token
        config.so_func_name = "decrypt_keystore_by_key";
        config.token = default_miner.at("token");
        int libret = load_lib(config);
        if (libret != 0) {
            std::cout << "decrypt keystore:" << config.keystore_path << " failed" << std::endl;
            return false;
        }

        return true;
    }

    bool default_key_flag = false;
    if (default_miner.size() >= 3 && !default_miner.at("node_id").empty() && !default_miner.at("public_key").empty() && !default_miner.at("path").empty()) {
        // maybe already setminer key, but get private_key from safebox failed, try decrypt using empty password
        config.node_id = default_miner.at("node_id");
        config.pub_key = default_miner.at("public_key");
        config.keystore_path = default_miner.at("path");
    } else {
        auto keystore_dir = config.datadir + "/keystore";
        std::vector<std::map<std::string, std::string>> keystore_vec;
        if (!scan_keystore_dir(keystore_dir, keystore_vec)) {
            std::cout << "No Account found, please using 'topio wallet createKey' to create account first" << std::endl;
            return false;
        }

        if (keystore_vec.size() > 1) {
            std::cout << "Please set a miner key by command 'topio mining setMinerKey'." << std::endl;
            return false;
        }

        // only one keystore file found, than that is the default keystore
        config.node_id       = keystore_vec[0].at("account");
        config.pub_key       = keystore_vec[0].at("public_key");
        config.keystore_path = keystore_vec[0].at("path");
        default_key_flag = true;
    }

    config.so_func_name = "decrypt_keystore";
    int libret = load_lib(config);
    if (libret != 0) {
        if (default_key_flag) {
            std::cout << "Please set a miner key by command 'topio mining setMinerKey'" << std::endl;
        } else {
            std::cout << "decrypt keystore:" << config.keystore_path << " failed" << std::endl;
        }
        return false;
    }

    return true;
}


// datadir not exist will create it first
bool datadir_check_init(config_t& config) {
    char* topio_home = getenv(TOPIO_HOME_ENV.c_str());
    if (!topio_home) {
        // if TOPIO_HOME_ENV is not set, using $HOME/Topnetwork as datadir
        struct passwd *pw = NULL;
        struct group *grp = NULL;

        // get current user info and group info
        do {
            pw = (struct passwd *)getpwuid(getuid());
            if (NULL == pw) {
                // no password entry
                break;
            }

            /*
            printf("login name : %s\n", pw->pw_name);
            printf("user id : %d\n", (int)pw->pw_uid);
            printf("group id : %d\n", (int)pw->pw_gid);
            printf("home dir : %s\n", pw->pw_dir);
            printf("login shell : %s\n", pw->pw_shell);
            */
            config.datadir = pw->pw_dir;
            config.datadir += "/topnetwork";

            grp = (struct group *)getgrgid((gid_t)(pw->pw_gid));
            if (NULL == grp) {
                // not group entry
                break;
            }

            //printf("user group name : %s", (const char*)grp->gr_name);
        } while (0);
    } else {
        // TOPIO_HOME_ENV is set, using as datadir
        config.datadir = topio_home;
    }

    if (config.datadir.back() == '/') {
        config.datadir.pop_back();
    }

    std::string log_path = config.datadir + "/log";
    config.com_log_path = log_path;
    config.pid_file = config.datadir + "/topio.pid";
    config.safebox_pid = config.datadir + "/safebox.pid";

    if (!isDirExist(config.datadir)) {
        int mk_status = mkdir(config.datadir.c_str(), 0755);
        if (mk_status != 0) {
            if (errno == EACCES) {
                std::cout << "mkdir path:" << config.datadir << " failed, Permission denied" << std::endl;
            } else {
                std::cout << "mkdir path:" << config.datadir << " failed" << std::endl;
            }
            return false;
        }
    } else {
        // if dir exist, judge write access
        int a_status = access(config.datadir.c_str(), W_OK);
        if (a_status != 0) {
            std::cout << "Permission denied to access path:" << config.datadir << std::endl;
            return false;
        }
    }

    // do not need care about return code, because it's not important
    mkdir(config.com_log_path.c_str(), 0755);
    return true;
}

std::string get_simple_version() {
    return std::string(PROGRAM_VERSION);
}

//get current dir
std::string get_working_path()
{
   char temp[256];
   return ( getcwd(temp, sizeof(temp)) ? std::string( temp ) : std::string("") );
}

// start http server for password cache.
int StartNodeSafeBox(const std::string& safebox_addr, uint16_t safebox_port, std::string const& pid_file) {
    // register signal
    if (register_signals(false) != 0) {
        std::cerr << "StartNodeSafeBox register signals failed" << std::endl;
        return -1;
    }

    if (!daemon()) {
        std::cout << "create daemon process failed" << std::endl;
        return -1;
    }

    // set pid
    auto pid = getpid();
    std::ofstream out_pid(pid_file);

    if (!out_pid.is_open()) {
        std::cout << "dump xnode pid:" << pid << " to file:" << pid_file << " failed\n";
        return -1;
    }

    out_pid << std::to_string(pid);
    out_pid.flush();
    out_pid.close();
    safebox_pid = pid_file;

    // set process title
    std::string new_title = "node safebox process";
    if (topio_init_setproctitle() == 0) {
        topio_setproctitle(new_title.c_str());
    }

    // now is daemon

    auto safebox_http_server = std::make_shared<safebox::SafeboxHttpServer>(safebox_addr, safebox_port);
    if (!safebox_http_server) {
        return -1;
    }

    try {
        // will block here
        safebox_http_server->Start();
    } catch (...) {
        std::cout << "start safebox http error" << std::endl;
        return -1;
    }
    return 0;
}

int ReloadNode(config_t& config) {
    std::ifstream in_pid(config.pid_file);
    if (!in_pid.is_open()) {
        std::cout << "open xnode pid_file:" << config.pid_file << " failed" << std::endl;
        return false;
    }

    char buff[16];
    bzero(buff, sizeof(buff));
    in_pid.getline(buff, 16);
    uint32_t xnode_pid = std::stoi(std::string(buff));
    in_pid.close();
    std::cout << "found runing xnode_pid:" << xnode_pid << std::endl;

    std::cout << "will send SIGHUP signal to pid:" << xnode_pid << std::endl;
    if (kill(xnode_pid, SIGHUP) == -1) {
        std::cout << "kill(SIGHUP) failed:" << strerror(errno) << "," << xnode_pid << "," << getpid() << std::endl;
        return false;
    }
    std::cout << "topio:" << xnode_pid << " will reload after seconds" << std::endl;
    return true;
}

int StopNode(config_t& config) {
    std::ifstream in_pid(config.pid_file);
    if (!in_pid.is_open()) {
        std::cout << "open xnode pid_file:" << config.pid_file << " failed" << std::endl;
        return false;
    }

    char buff[16];
    bzero(buff, sizeof(buff));
    in_pid.getline(buff, 16);
    uint32_t xnode_pid = std::stoi(std::string(buff));
    in_pid.close();
    std::cout << "found runing topio pid:" << xnode_pid << std::endl;

    //std::cout << "will send SIGKILL signal to pid:" << xnode_pid << std::endl;
    // will exit xnode process
    if (kill(xnode_pid, SIGHUP) == -1) {
        std::cout << "kill(SIGHUP) failed:" << strerror(errno) << "," << xnode_pid << "," << getpid() << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    // will exit topio process
    if (kill(xnode_pid, SIGTERM) == -1) {
        std::cout << "kill(SIGKILL) failed:" << strerror(errno) << "," << xnode_pid << "," << getpid() << std::endl;
        return false;
    }
    std::cout << "Stop the node successfully." << std::endl;
    return true;
}

// old way, xtopchain mode
int StartNodeWithConfig(config_t& config) {
    if (config.config_file.empty()) {
        std::cout << "empty config_file for topio(xtopchain mode)" << std::endl;
        return -1;
    }
    std::cout << "======================================================start topio in xtopchain mode===================================================================" << std::endl;
    config.so_func_name = "init_component";
    return load_lib(config);
}

int StartNode(config_t& config) {
    if (check_process_running(config.pid_file)) {
        std::cout << "topio already running, Aborting!" << std::endl;
        return -1;
    }


    if (init_log() != 0) {
        std::cout << "init_log failed" << std::endl;
        return -1;
    }


    if (config.config_file.empty()) { //load from keystore
        if (!load_keystore(config)) {
                //std::cout << "decrypt_keystore failed, exit" << std::endl;
                return -1;
        }

        if (!config.genesis) {
            // check miner info
            config.so_func_name = "check_miner_info";
            int miner_status = load_lib(config);
            if (miner_status != 0) {
                // check miner info not ok: not registered or pub key not matched
                std::cout << "Start node failed." << std::endl;
                return false;
            }
        }

    } else { //read from config
        auto & topio_config = top::topio::xtopio_config_t::get_instance();
        topio_config.load_config_file(config.config_file);
        config.node_id =  topio_config.get_string("node_id");
        config.pub_key = topio_config.get_string("public_key");
        std::cout << "node_id: " << config.node_id << "\n";
        std::cout << "public_key: " << config.pub_key << "\n";
    }


#ifdef DEBUG
    std::cout << "datadir:       " << config.datadir << std::endl
        << "node_id:       " << config.node_id << std::endl
        << "public_key:    " << config.pub_key << std::endl;
#endif

    generate_extra_config(config);

    // judge if using config file
    if (!config.config_file.empty()) {
        // using config
        int res = StartNodeWithConfig(config);
        if (res != 0) {
            std::cout << "Start node failed!\n";
            return res;
        }

        std::cout << "Start node successfully.\n";
        return 0;
    }

    if (config.daemon) {
        std::cout << "Start node successfully." << std::endl;
        // start as daemon process
        if (!daemon()) {
            std::cout << "create daemon process failed" << std::endl;
            return -1;
        } else {
            std::cout << "daemon process started" << std::endl;
        }
    }

    uint32_t xnode_pid = getpid();
    std::ofstream out_pid(config.pid_file);
    if (!out_pid.is_open()) {
        std::cout << "dump xnode pid:" << xnode_pid << " to file:" << config.pid_file << " failed" << std::endl;
        return -1;
    }

    out_pid << std::to_string(xnode_pid);
    out_pid.flush();
    out_pid.close();
    pid_file = config.pid_file;

    config.so_func_name = "init_noparams_component";
    if (!config.multiple_process) {
        start_single_process_mode(config);
    } else {
        start_master_worker_mode(config);
    }
    return 0;
}


int ExecuteCommands(int argc, char* argv[], config_t& config) {
    config.so_func_name = "parse_execute_command";
    /*
    config.params_line = "";
    for (int i = 0; i < argc; i++) {
        if (strlen(argv[i]) == 0) {
            continue;
        }

        config.params_line += argv[i];
        config.params_line += " ";
    }
    */
    config.argc = argc;
    config.argv = argv;

    return load_lib(config);
}

int filter_node_commandline(config_t &config, int argc, char *argv[]) {
    if (argc == 1) {
        return 1;
    }
    for (auto i = 0; i < argc; ++i) {
        std::string tmp(argv[i]);
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
        if (tmp == "-h" || tmp == "--help") {
            // filter "node [subcommand] -h"
            return 1;
        }
        if (i == 1 && tmp != "node") {
            return 1;
        }
        if (i == 2) {
            if (tmp != "startnode" && tmp != "stopnode" && tmp != "reloadnode" && tmp != "safebox") {
                return 1;
            }
        }
    }

    // only focus on "topio node [startNode/stopNode/reloadNode] [options not help]

    CLI::App app{"topio - the cpp-topnetwork command line interface"};
    app.ignore_case();
    // require at least one command
    app.require_subcommand(1);
    /*
    auto version_func =  [&]() -> void {
        config.so_func_name = "get_version";
        process_version_operation(config);
        exit(0);
    };
    app.add_flag_callback("-v,--version", version_func, "topio version");
    */

    // node
    auto node = app.add_subcommand("node", "node command");
    node->require_subcommand(1);
    // startnode
    auto startnode = node->add_subcommand("startNode", "start topio");
    int single_process_flag;
    startnode->add_flag("-S,--single_process", single_process_flag, "start topio as single_process(default master+worker mode)");
    //startnode->add_option("--cpu_net_interval", config.cpu_net_interval, "the interval of monitoring cpu and net (default: 10)");
    startnode->add_option("--admin_http_addr", config.admin_http_addr, "admin http server addr(default: 127.0.0.1)");
    startnode->add_option("--admin_http_port", config.admin_http_port, "admin http server port(default: 8000)");
    startnode->add_option("--bootnodes", config.bootnodes, "Comma separated endpoints(ip:port) for P2P  discovery bootstrap");
    startnode->add_option("--net_port", config.net_port, "p2p network listening port (default: 9000)");
    startnode->add_option("-c,--config", config.config_file, "start with config file");
    int32_t genesis_flag;
    startnode->add_flag("-g,--genesis", genesis_flag, "start with genesis mode");
    int32_t nodaemon_flag;
    startnode->add_flag("--nodaemon", nodaemon_flag, "start as no daemon");
    startnode->callback([&]()-> int {
            if (single_process_flag == 1) {
                config.multiple_process = false;
            }
            if (nodaemon_flag == 1) {
                config.daemon = false;
            }
            if (genesis_flag == 1) {
                config.genesis = true;
            }

            return StartNode(config);
            });

    // stopnode
    auto stopnode = node->add_subcommand("stopNode", "stop topio");
    stopnode->callback([&]()-> int {
            return StopNode(config);
            });

    // reloadnode
    auto reloadnode = node->add_subcommand("reloadNode", "reload topio");
    reloadnode->callback([&]()-> int {
            return ReloadNode(config);
            });

    auto safeboxnode = node->add_subcommand("safebox", "safebox process, manager accounts and token");
    std::string safebox_addr = safebox::safebox_default_addr;
    uint16_t safebox_port = safebox::safebox_default_port;
    safeboxnode->add_option("--http_addr", safebox_addr, "safebox http server addr(default: 127.0.0.1)");
    safeboxnode->add_option("--http_port", safebox_port, "safebox http server port(default: 7000)");
    safeboxnode->callback([&]() -> int {
            return StartNodeSafeBox(safebox_addr, safebox_port, config.safebox_pid);
            });

    try {
        app.parse(argc, argv);
    } catch(const CLI::ParseError &e) {
        app.exit(e);
        return -1;
    }

    return 0;
}

int main(int argc, char * argv[]) {
    topio_os_argv = argv;
    topio_os_argc = argc;
    config_t config;

    if(!datadir_check_init(config)) {
        std::cout << config.datadir << " invalid, please check" << std::endl;
        return -1;
    }


    if (auto r = filter_node_commandline(config, argc, argv) != 1) {
        return r;
    }

    generate_extra_config(config);

    // handle other(topcl/xnode/db) commands
    auto estatus  = ExecuteCommands(argc, argv, config);

    if (!check_process_running(config.safebox_pid)) {
        // every command will try to load safebox http server
        StartNodeSafeBox(safebox::safebox_default_addr, safebox::safebox_default_port, config.safebox_pid);
    }

    return estatus;
} // end main

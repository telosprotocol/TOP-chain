#pragma once

#include <vector>
#include <functional>
#include <map>
#include <mutex>

#include "xelect_net/include/multilayer_network_interface.h"
#include "xsyncbase/xsync_face.h"

namespace top {

using XchainArguments = std::vector<std::string>;
using XchainCommandProc = std::function<void (const XchainArguments &, const std::string &cmdline, std::string &result)>;
using XchainMapCommands = std::map<std::string, XchainCommandProc>;

class ChainCommands {
public:
    ChainCommands(ChainCommands const &)             = delete;
    ChainCommands & operator=(ChainCommands const &) = delete;
    ChainCommands(ChainCommands &&)                  = delete;
    ChainCommands & operator=(ChainCommands &&)      = delete;
    virtual ~ChainCommands()                         = default;
    ChainCommands()                                  = default;

    explicit ChainCommands(elect::MultilayerNetworkInterfacePtr net_module, sync::xsync_face_t *sync);
    ChainCommands(const std::string &datadir, elect::MultilayerNetworkInterfacePtr net_module, sync::xsync_face_t *sync);
    //ChainCommands(MultilayerNetworkInterfacePtr net_ptr, other module ptr);

    bool ProcessCommand(const std::string &cmdline, std::string &result);

private:
    void AddNetModuleCommands();
    void AddSyncModuleCommands();
    void AddCommand(const std::string &module_name, const std::string &sub_cmd_name, XchainCommandProc cmd_proc);
    bool StartIpcServerLoop(const std::string &sock_file);

    elect::MultilayerNetworkInterfacePtr net_module_;
    sync::xsync_face_t *sync_module{};

    std::mutex map_commands_mutex_;
    XchainMapCommands map_commands_;
};

typedef std::shared_ptr<ChainCommands> ChainCommandsPtr;


//  begin handle chain command (net,sync,db)
std::string get_working_path();
int db_backup( const std::string &from, const std::string &to);
int db_restore(const std::string &from, const std::string &to,const int backupid);
int db_download(const std::string datadir, const std::string& download_addr, std::ostringstream& out_str);
bool isDirExist(std::string dirPath);
bool isFileExist (const std::string &name);
bool IsDirEmpty(const char *dirname);
int multiplatform_mkdir(const std::string &path);

int parse_execute_command(const char *config_file_extra, int argc, char *argv[]);
bool handle_chain_command(
        const std::pair<std::string, uint16_t> &admin_http,
        const std::string &cmdline,
        std::string &result);
bool handle_node_command(
        const std::pair<std::string, uint16_t> &admin_http,
        const std::string &cmdline,
        std::string &result);

std::string decrypt_keystore_by_key(
        const std::string &keystore_path,
        const std::string &token);

}  //  namespace top

// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xupgrade.h"
#include <unistd.h>
#include <fstream>
#include <thread>
#include <openssl/md5.h>
#include <string.h>

#include "u_string.h"
#include "u_system.h"
#include "u_https_client.h"
#include "u_cmd_server.h"
#include "cJSON.h"

static uint32_t VRF_BUCKET_NUMBER = 4*60;  // four hour, 240 minutes

int json_parse_download_url(const std::string& return_https, std::string& download_url, std::string& md5sum, std::string& version) {
    cJSON* pRoot = cJSON_CreateObject();
    cJSON* pArray = cJSON_CreateArray();
    pRoot = cJSON_Parse(return_https.c_str());
    pArray = cJSON_GetObjectItem(pRoot, "assets");
    if (NULL == pArray) {
        return -1;
    }
    int iCount = cJSON_GetArraySize(pArray);
    int i = 0;
    for (; i < iCount; ++i) {
        cJSON* pItem = cJSON_GetArrayItem(pArray, i);
        if (NULL == pItem){
            continue;
        }
        char *url = cJSON_GetObjectItem(pItem, "browser_download_url")->valuestring;
        download_url = url;
    }
    md5sum = cJSON_GetObjectItem(pRoot, "body")->valuestring;
    version = md5sum;
    std::string::size_type found = md5sum.find("md5sum");
    if (found != std::string::npos){
        md5sum = md5sum.substr(found+6);
        md5sum = u_string::trim(md5sum);
        found = md5sum.find("\r\n");
        if (found != std::string::npos){
            md5sum.erase(found);
        }
    }

    found = version.find("version");
    if (found != std::string::npos){
        version = version.substr(found+7);
        version = u_string::trim(version);
        found = version.find("\r\n");
        if (found != std::string::npos){
            version.erase(found);
        }
    }

    cJSON_Delete(pRoot);
    return 0;
}
int json_parse_config(const std::string& ret, upgrade_config_t& config) {
    cJSON* pRoot = cJSON_CreateObject();
    pRoot = cJSON_Parse(ret.c_str());
    config.enable_upgrade = cJSON_GetObjectItem(pRoot, "enable_upgrade")->valueint;
    config.upgrade_mode = 0;
    if (strcmp(cJSON_GetObjectItem(pRoot, "upgrade_mode")->valuestring, "xnode") == 0) {
        config.upgrade_mode = 0;
    } else {
        config.upgrade_mode = 1;
    }
    config.https_addr = cJSON_GetObjectItem(pRoot, "https_addr")->valuestring;
    config.check_interval = cJSON_GetObjectItem(pRoot, "check_interval")->valueint;
    config.cmd_port = cJSON_GetObjectItem(pRoot, "cmd_port")->valueint;
    config.xchain_port = cJSON_GetObjectItem(pRoot, "xchain_port")->valueint;
    config.xchain_dir = cJSON_GetObjectItem(pRoot, "xchain_dir")->valuestring;
    config.local_ip = cJSON_GetObjectItem(pRoot, "local_ip")->valuestring;

    cJSON_Delete(pRoot);
    return 0;
}

int NeedUpgrade(const upgrade_config_t& config, std::string& md5sum, std::string& version) {
    char ret_data[1024];
    if (config.upgrade_mode == 0) {
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "%s/xnode -v | grep 'xtopchain version' | awk '{print $3}'", config.xchain_dir.c_str()) <= 0) {
            printf("get version error!\n");
            return 0;
        }
    }else {
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "%s/xtopchain -v | grep 'xtopchain version' | awk '{print $3}'", config.xchain_dir.c_str()) <= 0) {
            printf("get version error!\n");
            return 0;
        }
    }

    if (version.compare(std::string(ret_data)) != 0) {
        printf("version not equal:%s,%s\n", ret_data, version.c_str());
        return 1;
    }
    printf("version equal:%s,%s\n", ret_data, version.c_str());

    return 0;
}
int GenVrfHash(const upgrade_config_t& config, uint32_t& vrf_hash) {
    std::string strtemp;
    unsigned  char  md[16];
    strtemp = config.local_ip + ":" + std::to_string(config.xchain_port) + ":" + std::to_string(getpid()) ;
    MD5((const unsigned char *)strtemp.c_str(), strtemp.size(),md);
    vrf_hash = *(uint32_t*)(md + 12);
    printf("vrf hash: %s, md5:%s,%x\n", u_string::HexEncode(strtemp).c_str(), u_string::HexEncode(std::string((const char*)md, 16)).c_str(), vrf_hash);
    return 0;
}
int VrfOk(const upgrade_config_t& config, uint32_t vrf_hash) {
    time_t nSeconds;
    struct tm * pTM;

    time(&nSeconds); // nSeconds = time(NULL);
    pTM = localtime(&nSeconds);

    uint32_t bucket;
    bucket = (pTM->tm_min + 60 * pTM->tm_hour) % VRF_BUCKET_NUMBER;

    printf("time bucket:%d, vrf bucket:%d\n", bucket, vrf_hash % VRF_BUCKET_NUMBER);
    if (bucket == vrf_hash % VRF_BUCKET_NUMBER) {
        printf("\nvrf ok, start upgrade...\n");
        return 1;
    }
    return 0;
}
int UpgradeXchain(const upgrade_config_t& config) {
    char ret_data[1024];
    printf("kill xtopchain.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "kill -9 $(pidof xtopchain)") <= 0) {
    }
    printf("kill ok.\n");
    printf("backup old.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "rm -rf %s/xtopchain.bak;mv %s/xtopchain %s/xtopchain.bak", config.xchain_dir.c_str(), config.xchain_dir.c_str(), config.xchain_dir.c_str()) <= 0) {
    }
    printf("backup ok.\n");
    printf("repace xtopchain.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "mv /tmp/xtopchain %s/xtopchain;chmod +x %s/xtopchain", config.xchain_dir.c_str(), config.xchain_dir.c_str()) <= 0) {
    }
    printf("replace ok.\n");
    printf("start xtopchain.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "cd %s;rm -rf ./db ./db_store /tmp/log/*;./xtopchain ./config/config.json >output.log 2>&1 &", config.xchain_dir.c_str()) <= 0) {
    }
    printf("start ok.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "%s/xtopchain -v | grep 'xtopchain version' | awk '{print $3}'", config.xchain_dir.c_str()) <= 0) {
        printf("get version error!\n");
        sleep(config.check_interval);
        return 1;
    }
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "pidof xtopchain", config.xchain_dir.c_str()) <= 0) {
        printf("startup error, rollback.\n");
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "cd %s;mv xtopchain.bak xtopchain", config.xchain_dir.c_str()) <= 0) {
        }
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "cd %s;rm -rf ./db ./db_store /tmp/log/* xtopchain;./xtopchain ./config/config.json >output.log 2>&1 &", config.xchain_dir.c_str()) <= 0) {
        }
        sleep(config.check_interval);
        return 1;
    }
    printf("upgrade over. current version:%s\n\n", ret_data);
    return 0;
}
int UpgradeXnode(const upgrade_config_t& config) {
    char ret_data[1024];
    printf("backup old.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "rm -rf %s/libxtopchain.so.bak;mv %s/libxtopchain.so %s/libxtopchain.so.bak", config.xchain_dir.c_str(), config.xchain_dir.c_str(), config.xchain_dir.c_str()) <= 0) {
    }
    printf("backup ok.\n");
    printf("repace xtopchain.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "mv /tmp/libxtopchain.so %s/libxtopchain.so;chmod +x %s/libxtopchain.so", config.xchain_dir.c_str(), config.xchain_dir.c_str()) <= 0) {
    }
    printf("replace ok.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "cd %s;rm -rf ./db/*", config.xchain_dir.c_str()) <= 0) {
    }

    printf("kill xtopchain.\n");
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "kill -9 $(ps -ef | grep xnode |grep -v grep| grep -v '^root ' | awk '{print $2}')") <= 0) {
    }
    printf("kill ok.\n");

    sleep(10);  // wait for 10s to start xtopchain
    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "%s/xnode -v | grep 'xtopchain version' | awk '{print $3}'", config.xchain_dir.c_str()) <= 0) {
        printf("get version error!\n");
        sleep(config.check_interval);
        return 1;
    }

    if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "ps -ef | grep xnode |grep -v grep| grep -v '^root ' | awk '{print $2}'", config.xchain_dir.c_str()) <= 0) {
        printf("startup error, rollback.\n");
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "cd %s;mv libxtopchain.so.bak libxtopchain.so", config.xchain_dir.c_str()) <= 0) {
        }
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "cd %s;rm -rf ./db/*", config.xchain_dir.c_str()) <= 0) {
        }
        if (u_system::popen_fill(ret_data, sizeof(ret_data), 0, "kill -9 $(ps -ef | grep xnode |grep -v grep| grep -v '^root ' | awk '{print $2}')") <= 0) {
        }
        sleep(config.check_interval);
        return 1;
    }
    printf("upgrade over. current version:%s\n\n", ret_data);
    return 0;
}

int main(int argc, char** argv) {
    std::string return_https;
    std::string config_file;
    int res;
    while((res = getopt(argc , argv , "u:")) != -1){
        switch(res){
            case 'u':
                config_file = optarg;
                break;
        }
    }
    if (config_file.empty()) {
        printf("please input config file.\n");
        return 0;
    }
    char ret[1024];
    std::ifstream is(config_file);
    is.read(ret, sizeof(ret));
    printf("config,%d:%s\n", is.gcount(), ret);
    is.close();

    upgrade_config_t config;
    if (json_parse_config(ret, config) != 0) {
        printf("json parse error.\n");
        return 0;
    }
    printf("enable:%d,addr:%s,check_interval:%d,cmd_port:%d, xchain_port:%d, local_ip:%s\n",
        config.enable_upgrade, config.https_addr.c_str(), config.check_interval, config.cmd_port, config.xchain_port, config.local_ip.c_str());

    std::thread cmd_thread(start_cmd_server, std::ref(config));
    cmd_thread.detach();

    uint32_t vrf_hash;
    GenVrfHash(config, vrf_hash);

    int wait_upgrade = 0;
    std::string wait_version;
    char ret_data[1024];

    while (1) {
        if (config.enable_upgrade == 0) {
            sleep(config.check_interval);
            continue;
        }
        if (config.https_addr.empty()) {
            printf("please set https address.\n");
            return 0;
        }
        printf("start get github addr\n");
        if (get_https(config.https_addr, return_https) != 0) {
            printf("get https error\n");
            sleep(config.check_interval);
            continue;
        }
        printf("\nrecv,%d:%s\n", return_https.size(), return_https.c_str());

        std::string download_url, md5sum, version;
        if (json_parse_download_url(return_https, download_url, md5sum, version) != 0) {
            printf("json parse error.\n");
            return 0;
        }
        printf("download_url:%s\n", download_url.c_str());
        printf("md5sum:%s\n", md5sum.c_str());
        printf("version:%s,wait_version:%s\n\n", version.c_str(), wait_version.c_str());

// wait_upgrade = 0, not download any version
// wait_upgrade = 1, version != wait_version, already download old version, need to download new version
// wait_upgrade = 1, version == wait_version, download the newest version
        if (wait_upgrade == 1 && version == wait_version) {
            if (!VrfOk(config, vrf_hash)) {
                printf("wait for next vrf time.\n\n");
                sleep(config.check_interval);
                continue;
            }

            if (config.upgrade_mode == 1) {
                if (UpgradeXchain(config) == 1)
                    continue;
            } else if (config.upgrade_mode == 0) {
                if (UpgradeXnode(config) == 1)
                    continue;
            }
            wait_upgrade = 0;
            wait_version.clear();
            sleep(config.check_interval);
            continue;
        }
        if (NeedUpgrade(config, md5sum, version) == 0){
            sleep(config.check_interval);
            continue;
        }

        std::string tmpFile;
        srand((int)time(0));
        tmpFile = std::string("tmp_xtopchain") + std::to_string(rand());
        if (u_system::popen_fill(ret_data, 1024, 0, "wget %s -O /tmp/%s", download_url.c_str(), tmpFile.c_str()) <= 0) {
        }
        if (u_system::popen_fill(ret_data, 1024, 0, "cd /tmp;rm -rf xtopchain libxtopchain.so;tar zxvf %s", tmpFile.c_str()) <= 0) {
            printf("tar extract error.\n");
            sleep(config.check_interval);
            continue;
        }
        printf("tar extract ok:%s\n", ret_data);
        if (u_system::popen_fill(ret_data, 1024, 0, "md5sum /tmp/%s | awk '{print $1}'", tmpFile.c_str()) <= 0) {
            printf("md5sum fail:%s\n", ret_data);
            sleep(config.check_interval);
            continue;
        }
        printf("md5sum local:%s\n", ret_data);
        if (md5sum.compare(ret_data) != 0) {
            printf("md5sum not equal.\n");
            sleep(config.check_interval);
            continue;
        }
        wait_upgrade = 1;
        wait_version = version;
    }

    return 0;
}

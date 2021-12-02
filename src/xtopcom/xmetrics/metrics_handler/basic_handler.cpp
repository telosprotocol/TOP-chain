// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "basic_handler.h"

NS_BEG3(top, metrics, handler)

top::base::xlogger_t *g_metrics_log_instance = NULL;

void metrics_log_init(const std::string& log_path)
{
    //init metrics log
    if(NULL == g_metrics_log_instance) {
        g_metrics_log_instance = top::base::xlogger_t::create_instance("xmetric",log_path);
        if(g_metrics_log_instance) {
             std::cout << "g_metrics_log_instance create sucess!"<< log_path.c_str()<<std::endl;
            g_metrics_log_instance->set_log_level(enum_xlog_level_info);
            g_metrics_log_instance->kinfo("[xmetrics]alarm start ");
        }else {
           std::cout << "g_metrics_log_instance create failed!"<<std::endl;
        }  
    }
}

void metrics_log_close()
{
    //close log
    if (g_metrics_log_instance) {
        g_metrics_log_instance->close();
    }      
}

NS_END3
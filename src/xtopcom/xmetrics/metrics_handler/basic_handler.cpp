#include "basic_handler.h"



top::base::xlogger_t *g_metrics_log_instance = NULL;

void metrics_log_init(const char* log_path)
{
    //init metrics log
    if(NULL == g_metrics_log_instance) {
        g_metrics_log_instance = xlogger_t::create_instance("xmetric",log_path);
        if(g_metrics_log_instance) {
             std::cout << "g_metrics_log_instance create sucess!"<< log_path<<std::endl;
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
    g_metrics_log_instance->close();
}


NS_BEG3(top, metrics, handler)


/*
 void xbasic_handler_t::dump(std::string const & str, bool is_updated) {
    bool dump_all{false};
    XMETRICS_CONFIG_GET("dump_full_unit", dump_all);
    if (is_updated || dump_all) {
        if(g_metrics_log_instance){
              g_metrics_log_instance->kinfo("[metrics]%s", str.c_str());
        }
        else{
            xkinfo("[metrics]%s", str.c_str());
        }   
        #ifdef METRICS_UNIT_TEST
             std::cout << str << std::endl;
         #endif
    }
}*/



 NS_END3
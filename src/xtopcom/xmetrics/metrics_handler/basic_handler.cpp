#include "basic_handler.h"


NS_BEG3(top, metrics, handler)


using namespace top::base;


top::base::xlogger_t *g_metrics_log_instance = nullptr;


 void xtop_basic_handler::dump(std::string const & str, bool is_updated) {
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
}


void metrics_log_init(std::string log_path)
{
    //init metrics log
    if(!g_metrics_log_instance) {
        g_metrics_log_instance = xlogger_t::create_instance("xmetric",log_path.c_str());
        if(g_metrics_log_instance) {
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


 NS_END3
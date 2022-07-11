#include "xtopcl/include/global_definition.h"
#include "xtopcl/include/topcl.h"
#include "xtopcl/include/user_info.h"

int main(int argc, const char ** argv) {
    top::xtopcl::xtopcl xtop_cl;
    xtop_cl.api.change_trans_mode(true);
    return 0;
}

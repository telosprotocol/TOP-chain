
#include <gtest/gtest.h>

static bool kStop = false;

void SigCatch(int sig_no) {
    if (SIGTERM == sig_no || SIGINT == sig_no) {
        kStop = true;
    }
}

int main(int argc, char** argv) {

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
        signal(SIGTERM, SigCatch) == SIG_ERR) {
        std::cout << "register signal handler failed!" << std::endl;
        return 1;
    }

    testing::InitGoogleTest(&argc, argv);
    int nRet = RUN_ALL_TESTS();

    while (!kStop) {
        sleep(20);
    }
    
    
    return 0;
}
#include <cstdio>
#include <signal.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);
    int nRet = RUN_ALL_TESTS();

    return nRet;
}

#pragma once

#include <memory>

namespace top {
namespace elect {

class ElectOption {
public:
    static ElectOption* Instance();

public:
    ElectOption();
    bool IsEnableStaticec();
    // call once at program init
    void EnableStaticec();

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

}
}

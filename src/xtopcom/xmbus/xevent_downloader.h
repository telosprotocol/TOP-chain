#pragma once

#include "xmbus/xevent.h"
#include "xdata/xblock.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

class xevent_downloader_t : public xevent_t {
public:
    enum _minor_type_ {
        none,
        complete,
    };

    xevent_downloader_t(_minor_type_ type)
    : xevent_t(xevent_major_type_downloader, type, to_listener, true) {
    }
};

DEFINE_SHARED_PTR(xevent_downloader);

class xevent_downloader_complete_t : public xevent_downloader_t {
public:
    xevent_downloader_complete_t(const std::string &_address,
            uint64_t _height):
    xevent_downloader_t(complete),
    address(_address),
    height(_height) {
    }

    std::string address;
    uint64_t height;
};


NS_END2

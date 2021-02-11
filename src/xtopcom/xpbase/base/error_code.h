#pragma once

namespace top {
namespace kadmlia {

enum RoutingReturnCode {
    kKadSuccess = 0,
    kKadFailed = 1,
    kKadTimeout = 2,
    kKadNodeNotExists = 3,
    kKadNodeHasAdded = 4,
    kKadDetected = 5,
    kKadNotExists = 6,
    kKadContinue = 7,
    kKadForbidden = 8,

    kSmartObjectStoreOk = 21,
    kSmartObjectInvalid = 22,

    kSmartObjectNotEnoughNodes = 31,
    kSmartObjectDataTypeError = 32,
    kSmartObjectDataNotExist = 33,
	kSmartObjectAuthNotOwner = 34,

    kDhtPingTestRequest = 50,
    kDhtPingTestResponse,

    kUdpNatFailed = 100,

};

}  // namespace kadmlia
}  // namespace top

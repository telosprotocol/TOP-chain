#pragma once

namespace top {

namespace elect {

enum ElectVhostSendErrorCode {
    kVhostSendSuccess = 0,
    kVhostSendFailed = 1,
    kVhostSendSrcInvalid = 2,
    kVhostSendDstInvalid = 3,
    kVHostSendWrouterFailed = 4,
};

enum SubNetworkType {
    kInvalid = 0,
    kElectionCommittee = 1,
    kBackupNetwork = 2,
};

}  // namespace elect

}  // namespace top

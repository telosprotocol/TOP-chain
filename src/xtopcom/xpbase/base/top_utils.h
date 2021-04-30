//
//  top_utils.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>
#include <random>
#include <mutex>
#include <algorithm>
#include <set>
#include <limits>


#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
        TypeName(const TypeName&); \
        TypeName& operator=(const TypeName&)
#endif  // !DISALLOW_COPY_AND_ASSIGN

namespace top {

namespace base {
    class KadmliaKey;
}
extern std::shared_ptr<base::KadmliaKey> global_xid;
extern uint32_t global_platform_type;
extern std::string global_node_id;
extern std::string global_node_id_hash;
extern std::string global_node_signkey;
//static const uint32_t kRoot = std::numeric_limits<uint32_t>::max(); // avoid confilict with xip
static const uint32_t kRoot = 0xFFFFFF; // avoid confilict with xip

enum RoutingTableType {
    kInvalidType = 0,
    kClient,
    kEdgeBackbone,

    // edge p2p
    kEdgeXVPN,       // attention: kEdgeXVPN must set 4 ,or client 0.5.0 can not work
    kEdgeTopMessage,
    kEdgeTopStorage,
    kDhtDemoEdge,    // for dht test,if delete ask smaug first

    // service p2p
    kXVPN,
    kTopStorage,
    kTopMessage,

    kChainRecNet = 100,
    kChainZecNet,
    kChainEdgeNet,
    kChainArchiveNet,
    kChainAdvanceGroup,
    kChainConsensusGroup,
};

enum NodeRoleType {
    kRoleInvalid = 0,
    kRoleEdge = 1,
    kRoleService = 2,
    kRoleClient = 3,
    kRoleConsensus = 4,
};

enum PlatformType {
    kChain = 0,
    kPlatform = 1,
};

static const int kNodeIdSize = 36;
static const uint32_t kMessageMaxPriority = 5;
static const uint32_t kMaxChannelSize = 3;
static const uint32_t kEnoughSuccessAckNum = 1;
static const uint32_t kEnoughSuccessAckTimeout = 3;
static const int64_t kRecursiveCount = 4;
static const uint32_t kUdpPacketBufferSize = 8000U;


static const std::string LOCAL_VPN_SERVER_DB_KEY = "local_vpn_server";
static const std::string LOCAL_EDGE_DB_KEY = "local_edge";
static const std::string LOCAL_COUNTRY_DB_KEY = "local_country_code";
static const std::string kLocalDbGlobalXidField = "global_xid";
static const std::string BLOCK_DB_KEY = "block_chain";
static const std::string BLOCK_DB_LAST_KEY = "block_chain_last_hash";
static const std::string BLOCK_DB_HEADER_KEY = "block_chain_head";
static const std::string BLOCK_DB_BODY_KEY = "block_chain_body";

static const char KEY_FIELD_SPLIT_STR = '\x01';
typedef std::function<void(const int&, const std::string&)> ReceiveCallbackFunctor;
static ReceiveCallbackFunctor GlobalVpnCallback;

uint64_t GetCurrentTimeMsec();
uint64_t GetCurrentTimeMicSec();
void TrimString(std::string& in_str);
bool IsNum(const std::string& s);
void SleepUs(uint64_t time_us);
void SleepMs(uint64_t time_ms);
bool IsBigEnd();
bool IsSmallEnd();
uint32_t StringHash(const std::string& str);

std::string RandomString(size_t size);
std::mt19937& random_number_generator();
std::mutex& random_number_generator_mutex();
std::string GetStringSha128(const std::string& str);
std::string GetStringSha256(const std::string& str);

int32_t RandomInt32();
uint32_t RandomUint32();
uint64_t RandomUint64();
uint16_t  RandomUint16();

template <typename String>
String GetRandomString(size_t size) {
    std::uniform_int_distribution<> distribution(0, 255);
    String random_string(size, 0);
    {
        std::lock_guard<std::mutex> lock(random_number_generator_mutex());
        std::generate(random_string.begin(), random_string.end(),
            [&] { return distribution(random_number_generator()); });
    }
    return random_string;
}

std::string RandomAscString(size_t size);

std::string HexEncode(const std::string& str);
std::string HexDecode(const std::string& str);
std::string Base64Encode(const std::string& str);
std::string Base64Decode(const std::string& str);
std::string HexSubstr(const std::string& str);
std::string Base64Substr(const std::string& str);
std::string GetGlobalXidWithNodeId(const std::string& node_id);

enum CountryCode {
    kCountry_AQ = 0,
    kCountry_BI = 1,
    kCountry_CF = 2,
    kCountry_TD = 3,
    kCountry_CG = 4,
    kCountry_RW = 5,
    kCountry_ZR = 6,
    kCountry_BZ = 7,
    kCountry_CR = 8,
    kCountry_SV = 9,
    kCountry_GT = 10,
    kCountry_HN = 11,
    kCountry_MX = 12,
    kCountry_NI = 13,
    kCountry_PA = 14,
    kCountry_KZ = 15,
    kCountry_KG = 16,
    kCountry_TJ = 17,
    kCountry_TM = 18,
    kCountry_UZ = 19,
    kCountry_AT = 20,
    kCountry_CZ = 21,
    kCountry_HU = 22,
    kCountry_LI = 23,
    kCountry_SK = 24,
    kCountry_CH = 25,
    kCountry_CN = 26,
    kCountry_JP = 27,
    kCountry_KP = 28,
    kCountry_KR = 29,
    kCountry_TW = 30,
    kCountry_HK = 31,
    kCountry_MO = 32,
    kCountry_DJ = 33,
    kCountry_ER = 34,
    kCountry_ET = 35,
    kCountry_KE = 36,
    kCountry_SO = 37,
    kCountry_TZ = 38,
    kCountry_UG = 39,
    kCountry_BY = 40,
    kCountry_EE = 41,
    kCountry_LV = 42,
    kCountry_LT = 43,
    kCountry_MD = 44,
    kCountry_PL = 45,
    kCountry_UA = 46,
    kCountry_KM = 47,
    kCountry_MG = 48,
    kCountry_MU = 49,
    kCountry_YT = 50,
    kCountry_RE = 51,
    kCountry_SC = 52,
    kCountry_CA = 53,
    kCountry_GL = 54,
    kCountry_PM = 55,
    kCountry_US = 56,
    kCountry_UM = 57,
    kCountry_DZ = 58,
    kCountry_EG = 59,
    kCountry_LY = 60,
    kCountry_MA = 61,
    kCountry_SD = 62,
    kCountry_TN = 63,
    kCountry_EH = 64,
    kCountry_MN = 65,
    kCountry_RU = 66,
    kCountry_DK = 67,
    kCountry_FO = 68,
    kCountry_FI = 69,
    kCountry_IS = 70,
    kCountry_NO = 71,
    kCountry_SJ = 72,
    kCountry_SE = 73,
    kCountry_AS = 74,
    kCountry_AU = 75,
    kCountry_CK = 76,
    kCountry_FJ = 77,
    kCountry_PF = 78,
    kCountry_GU = 79,
    kCountry_KI = 80,
    kCountry_MH = 81,
    kCountry_FM = 82,
    kCountry_NR = 83,
    kCountry_NC = 84,
    kCountry_NZ = 85,
    kCountry_NU = 86,
    kCountry_NF = 87,
    kCountry_MP = 88,
    kCountry_PW = 89,
    kCountry_PG = 90,
    kCountry_PN = 91,
    kCountry_SB = 92,
    kCountry_TK = 93,
    kCountry_TO = 94,
    kCountry_TV = 95,
    kCountry_VU = 96,
    kCountry_WF = 97,
    kCountry_WS = 98,
    kCountry_AR = 99,
    kCountry_BO = 100,
    kCountry_BR = 101,
    kCountry_CL = 102,
    kCountry_CO = 103,
    kCountry_EC = 104,
    kCountry_FK = 105,
    kCountry_GF = 106,
    kCountry_GY = 107,
    kCountry_PY = 108,
    kCountry_PE = 109,
    kCountry_SR = 110,
    kCountry_UY = 111,
    kCountry_VE = 112,
    kCountry_AF = 113,
    kCountry_BD = 114,
    kCountry_BT = 115,
    kCountry_IN = 116,
    kCountry_MV = 117,
    kCountry_NP = 118,
    kCountry_PK = 119,
    kCountry_LK = 120,
    kCountry_IO = 121,
    kCountry_BV = 122,
    kCountry_SH = 123,
    kCountry_GS = 124,
    kCountry_BN = 125,
    kCountry_KH = 126,
    kCountry_CX = 127,
    kCountry_CC = 128,
    kCountry_ID = 129,
    kCountry_LA = 130,
    kCountry_MY = 131,
    kCountry_MM = 132,
    kCountry_PH = 133,
    kCountry_SG = 134,
    kCountry_TH = 135,
    kCountry_VN = 136,
    kCountry_TP = 137,
    kCountry_AL = 138,
    kCountry_BA = 139,
    kCountry_BG = 140,
    kCountry_HR = 141,
    kCountry_GR = 142,
    kCountry_MK = 143,
    kCountry_RO = 144,
    kCountry_SI = 145,
    kCountry_YU = 146,
    kCountry_AM = 147,
    kCountry_AZ = 148,
    kCountry_BH = 149,
    kCountry_CY = 150,
    kCountry_GE = 151,
    kCountry_IR = 152,
    kCountry_IQ = 153,
    kCountry_IL = 154,
    kCountry_JO = 155,
    kCountry_KW = 156,
    kCountry_LB = 157,
    kCountry_OM = 158,
    kCountry_QA = 159,
    kCountry_SA = 160,
    kCountry_SY = 161,
    kCountry_TR = 162,
    kCountry_AE = 163,
    kCountry_YE = 164,
    kCountry_AD = 165,
    kCountry_GI = 166,
    kCountry_PT = 167,
    kCountry_ES = 168,
    kCountry_AO = 169,
    kCountry_BW = 170,
    kCountry_LS = 171,
    kCountry_MW = 172,
    kCountry_MZ = 173,
    kCountry_NA = 174,
    kCountry_ZA = 175,
    kCountry_SZ = 176,
    kCountry_ZM = 177,
    kCountry_ZW = 178,
    kCountry_VA = 179,
    kCountry_IT = 180,
    kCountry_MT = 181,
    kCountry_SM = 182,
    kCountry_TF = 183,
    kCountry_HM = 184,
    kCountry_AI = 185,
    kCountry_AG = 186,
    kCountry_AW = 187,
    kCountry_BS = 188,
    kCountry_BB = 189,
    kCountry_BM = 190,
    kCountry_VG = 191,
    kCountry_KY = 192,
    kCountry_CU = 193,
    kCountry_DM = 194,
    kCountry_DO = 195,
    kCountry_GD = 196,
    kCountry_GP = 197,
    kCountry_HT = 198,
    kCountry_JM = 199,
    kCountry_MQ = 200,
    kCountry_MS = 201,
    kCountry_AN = 202,
    kCountry_PR = 203,
    kCountry_KN = 204,
    kCountry_LC = 205,
    kCountry_VC = 206,
    kCountry_TT = 207,
    kCountry_TC = 208,
    kCountry_VI = 209,
    kCountry_BJ = 210,
    kCountry_BF = 211,
    kCountry_CM = 212,
    kCountry_CV = 213,
    kCountry_CI = 214,
    kCountry_GQ = 215,
    kCountry_GA = 216,
    kCountry_GM = 217,
    kCountry_GH = 218,
    kCountry_GN = 219,
    kCountry_GW = 220,
    kCountry_LR = 221,
    kCountry_ML = 222,
    kCountry_MR = 223,
    kCountry_NE = 224,
    kCountry_NG = 225,
    kCountry_ST = 226,
    kCountry_SN = 227,
    kCountry_SL = 228,
    kCountry_TG = 229,
    kCountry_BE = 230,
    kCountry_FR = 231,
    kCountry_DE = 232,
    kCountry_IE = 233,
    kCountry_LU = 234,
    kCountry_MC = 235,
    kCountry_NL = 236,
    kCountry_GB = 237,
    kCountry_UK = 238,
    kCountry_FX = 239,
};

enum RoutingMessageType {
    kInvalidMessageType = 0,

    kKadBootstrapJoinRequest,
    kKadBootstrapJoinResponse,
    kKadFindNodesRequest,
    kKadFindNodesResponse,
    kKadConnectRequest,
    kKadConnectResponse,
    kKadHandshake,
    kKadAck,
    kKadHeartbeatRequest,
    kKadHeartbeatResponse,
    kKadNatDetectRequest,
    kKadNatDetectResponse,
    kKadNatDetectHandshake2Node,
    kKadNatDetectHandshake2Boot,
    kKadNatDetectFinish,
    kKadDropNodeRequest,
    kKadMessageTypeMax,  // other message mast bigger than it

    kGossipBlockSyncInvalid = kKadMessageTypeMax + 1, // now is 30
    kGossipBlockSyncAsk = 19,
    kGossipBlockSyncAck = 20,
    kGossipBlockSyncRequest = 21,
    kGossipBlockSyncResponse = 22,
    kGossipMaxMessageType,

    kMessageTypeMin = kKadMessageTypeMax + 1000,   // now is 1029
    kUdpNatDetectRequest,
    kUdpNatDetectResponse,
    kUdpNatHeartbeat,
    kNotFullFindNodesRequest,
    kRootMessage,
    kNatDetectRequest,
    kNatDetectResponse,
    kNatDetectHandshake2Node,
    kNatDetectHandshake2Boot,
    kNatDetectFinish,
    //gossip
    kRumorTest,
    kBlockSync,
    kBlockSyncRequest,
    kBlockSyncResponse,
    kRecGossipRequest,
    kRecGossipSendDataRequest,
    kRecGossipSendDataRecursivelyRequest,
    kRecGossipSendDataRelayRequest,
    // fts
    kFtsSyncRequest,
    kFtsSyncResponse,
    kFtsLeaderBlock,
    kFtsJoinRequest,
    kPublicKeyRequest,
    kPublicKeyResponse,
    // test multi relay message (will delete some day)
    kTestMultiRelayRequest,
    kTestMultiRelayResponse,
    kTestWpingRequest,
    kTestWpingResponse,
    kTestWbroadcastRequest,
    kTestWbroadcastResponse,
    kKadBroadcastFromMultiChannelRequest,
    kKadBroadcastFromMultiChannelAck,
    kKadSendToFromRandomNeighborsRequest,
    kTestChainTrade,
    kTestSuperBroadcastRecv,
    kMessageTypeMax,

    kSmartObjectMessageTypeMin = kMessageTypeMax + 1,
    kSmartObjectStoreRelayRequest,
    kSmartObjectStoreRelayResponse,
    kSmartObjectStoreRequest,
    kSmartObjectStoreResponse,
    kSmartObjectFindValueRelayRequest,
    kSmartObjectFindValueRelayResponse,
    kSmartObjectFindValueRequest,
    kSmartObjectFindValueResponse,
    kSmartObjectRefreshKeyRequest,
    kSmartObjectRefreshKeyResponse,
    kSmartObjectRefreshDataRequest,
    kSmartObjectRefreshDataResponse,
    kSmartObjectStoreGetHashRelayRequest,
    kSmartObjectStoreGetHashRequest,
    kSmartObjectStoreGetHashResponse,
    kSmartObjectStoreSyncDataCommandRequest,
    kSmartObjectStoreSyncDataCommandResponse,
    kSmartObjectStoreSyncDataRequest,
    kSmartObjectStoreSyncDataResponse,
    kSmartObjectBootstrapSyncDataRequest,
    kSmartObjectBootstrapSyncDataResponse,
    kSmartObjectDelRelayRequest,
    kSmartObjectDelRequest,
    kSmartObjectDelResponse,
    kSmartObjectTestStoreRequest,
    kSmartObjectTestStoreResponse,
    kSmartObjectTestFindValueRequest,
    kSmartObjectTestFindValueResponse,
	kSmartObjectAuthAllowEncKeyRequest,
	kSmartObjectAuthAllowEncKeyResponse,
	kSmartObjectAuthAddFriendRequest,
	kSmartObjectAuthAddFriendResponse,
	kSmartObjectAuthShareDataRelayRequest,
	kSmartObjectAuthShareDataRequest,
	kSmartObjectAuthShareDataResponse,
	kSmartObjectAuthGetDataRelayRequest,
	kSmartObjectAuthGetDataRequest,
	kSmartObjectAuthGetDataResponse,
    kSmartObjectMessageTypeMax,

    // for xelect_net module TODO(smaug) adjust enum order
    kXelectNetMessageTypeMin = kMessageTypeMax + 100,
    kVnetworkCallback,
    kElectConsensusMessage,
    kTestRecMessage,
    kTestZecMessage,
    kRegZoneMessage,
    kElectVhostRumorP2PMessage = 1159,
    kElectVhostRumorGossipMessage = 1160,


    // for xedge module bitvpn
    kClientInterfaceMessageTypeMin = kMessageTypeMax + 2000,
    kEdgeInfoRequest,
    kEdgeInfoResponse,
    kEdgeNodesRequest,
    kEdgeNodesResponse,
    kClientInterfaceMessageTypeMax,

    // for xnetwork module
    kPerformanceMessageTypeMin = kMessageTypeMax + 3000,
    kRoundTripTimeRequest,
    kRoundTripTimeResponse,
    kRelayTestRequest,
    kRelayTestResponse,
    kTellBootstrapStopped,
    kGetGroupNodesRequest,
    kGetGroupNodesResponse,
    kGetEcBackupFromBootRequest,
    kGetEcBackupFromBootResponse,
    kGetSubMemberFromBootRequest,
    kGetSubMemberFromBootResponse,
    kGetAllNodesFromBootRequest,
    kGetAllNodesFromBootResponse,
    kBroadcastPerformaceTest,
    kBroadcastRecTest,
    kBroadcastZecTest,
    kUdperfStart,
    kUdperfTest,
    kUdperfFinish,
    kBroadcastPerformaceTestReset,
    kPerformanceMessageTypeMax,

};

enum SerializeProtocol {
    kSerializeProtocolProtobuf = 1,
    kSerializeProtocolMsgpack,
    kSerializeProtocolXbase,
};

}  // namespace top

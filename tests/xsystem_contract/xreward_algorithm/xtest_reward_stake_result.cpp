// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <sstream>

#define private public

#include "xconfig/xconfig_register.h"
#include "xdata/xelect_transaction.hpp"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xstandby_network_result.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xunit_service/xcons_face.h"
#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_reward_contract.h"

using namespace top;

#define DEFAULT_MIN_GROUP_SIZE (32)
#define DEFAULT_MAX_GROUP_SIZE (32)
#define GROUP_ELECT_COUNT (1000)

XINLINE_CONSTEXPR char const *  sys_contract_sharding_workload_addr{ "T20000MFGB3gFWSsMfEo9LrC7JEaj1gJTXaYfXny" };

bool operator==(xvip2_t const & p1, xvip2_t const & p2)
{
    if (p1.high_addr == p2.high_addr && p1.low_addr == p2.low_addr) {
        return true;
    } else {
        return false;
    }
}

static const std::vector<std::string> test_accounts = {
    "T00000Li18Pb8WuxQUphZPfmN6QCWTL9kwWPTns1",
    "T00000LawC9X7onZ23ftMfsBb8XtCHH7yHPbUWef",
    "T00000LZTVtSXAAHpw5L92Jw4VkatpX64YW6Sona",
    "T00000LfoXb1vXXQiPxyQbnHsUgZKrNfTqLj7Rmr",
    "T00000LUgUM5DcTb4Be4gUEwCWByhUf191Qu9nFr",
    "T00000LPSxv3AVu7Kb4oET1QsVzFDiofZHF2BBH4",
    "T00000LXtwPC88mTkBo5cSBZ6rFFwifnnvSU2sB3",
    "T00000LiYuDqviN9Kzi84t2DZhamN7E6tukogyv8",
    "T00000LewNgnEwG8E1FwxiqY6sb7et7B3VzKmr63",
    "T00000LSbd1EhPLc45TM8FnjFJFbjqUMaoeiiK7L",
    "T00000LZhN8dgiPu3UXAniFcMGddgsDTu6upfCvw",
    "T00000LYsCRMporiwbcv4THJzAtWhzpvgK2qysYU",
    "T00000LbJNSqcjeNtAb3oxMoTiv9TxMRK4F5AbBh",
    "T00000LfAgMFFHuVHixLqXtVhHf97ApjBpcFRiMK",
    "T00000LhV1uFjJwLCTNFapuLYKMsaiqiYnSX5hoK",
    "T00000LeCT6tj3eEE7Y1PPubHB95SuxVQt8UEmBu",
    "T00000LadFgEiL8jmeN3gHMgzQccHLDdsPkq4DGP",
    "T00000LeQftFqhhtPBcWDNYLHSMDjUzi4wZs3wLD",
    "T00000LQFCSEYeFzXYtLfTPRbm2dYVkRzdDJNu1o",
    "T00000LeQCRU2BtZ29KZ5SsPvifGsNo8MeoSDQn4",
    "T00000LTmVu99sw7snYC3oaSv5HZqmJ9g9dGD3Xv",
    "T00000LSrDkb5xT1HPxLC5YsBPsxxJRQUnSdBgAc",
    "T00000LYNVeHLfZD6TVQuqsGvpzQM5Ywx1yBdPKX",
    "T00000LLUocKMpyskr1bX1DdtrpiqKvy3oWY5yFW",
    "T00000LcNVKFikgix1SFdbSNru19CMpRcN3ikcjC",
    "T00000LLxttrYBMCDKCP5aiEqDaXjhBrJ1sV9mSq",
    "T00000LKRkkE3PanNsN6VWaQ9zG941gYaNLcsXPm",
    "T00000LNYDgKGbAMpSegkNn7VH1TgmEvHT3bsh9n",
    "T00000Lb3QjHwgP6stckHpWVPoakev3kWAEauhcP",
    "T00000LSkNiBexHjKqvuuDzX6CQSH2L5MDLZYZw1",
    "T00000Lc9tqkGpah9GfKfQCJZCByeVCp3BBxpUsG",
    "T00000LQf6B7usSkDeq1WycX6JoGMVUQfA4Fawto",
    "T00000LSBDQPnVvXUCzJLyvdKd1cPqpCFgZksfth",
    "T00000LcXFu9yDj3NxYWE2YSfKxeDo7PpigK1c2E",
    "T00000LUrznL9rbp9owuxfyRBFTsShfwS4BBzJew",
    "T00000Lg6qi4yT1BY2HjJh1RKcRTXcZN4AcuajWZ",
    "T00000LYL6TuoLyZP9L88nA1G5UTaxhsiZbKVLDK",
    "T00000LT4vk1Zt1JkAFcUwpvcYhde46dUR2ZeZsx",
    "T00000LLDRirM4pWLAzfs1SLnD8j25Rx9KWS43KJ",
    "T00000LevCP3gaHypGbZarCdZUrLCryXvrxM6Taj",
    "T00000LYvSqQkaEcm952A7u16PhPDjNmXpqZ6nW2",
    "T00000LeX78Rymjnt2LTdjkvMWqWHyBR4PmkVy4k",
    "T00000LSnwAtBV7bnb5ASLpoCkCMvZrh19JynZSG",
    "T00000LZp5MtigSGBAfnKoZ5VHYwuY3yz14H1YaY",
    "T00000LckQSSYGvm2nqvBGKR6k7rkcnoZbGHJcRr",
    "T00000LS4ubZfQAhdMVjqxtAbJxmi6MnY91jqUww",
    "T00000LVhE9KjZ4cxsQ5dxNQ3NjVQTx9Hr9BiE98",
    "T00000LhnFCkZ9ShxABVdsdWqkV9ihuJ1Sxim1wz",
    "T00000Lcoi5MBsxPU9GyBeJL29abksoEGewzBACm",
    "T00000LePGLhYtnNGEmUq4MwitS27SqpoVt1anqe",
    "T00000LTby2SWekxyzw24gsnJTFMHEf4UBPs58d6",
    "T00000LhxdfkbVgjr6f8QAXQ59J1JtwSWvRwocc6",
    "T00000LbJfJFoRjsEMyDe47Qdghrb15A94aZMvZw",
    "T00000LKFafZ8m7afDvotwnGUk1Gazr228K18212",
    "T00000LfdLLJgZQo455Wc8LBpQUpERLvjhnvsFdB",
    "T00000LQMRinmFmBaP5P5uJRqVm1XpXdcTByHtEp",
    "T00000LXe7o9fZmhJFBdwRfCkzQ8nhHZDsVs1YqA",
    "T00000LXFLb3vZvaP97TRFZWZbXcct1nvCHQw4CZ",
    "T00000LUSBUWnZpE2kAfJKGFQ3xWWbrWPrGd8b61",
    "T00000LRVy35KjgqhzLSmFTyfZyenPZ1f9VvdYK2",
    "T00000LbTNWrrv2nT3TEdQv3ozNFhF25d7BooFhh",
    "T00000Lc2DJEqsLbX5a8ftmahTBoBo5vvuDQmpzF",
    "T00000LMMd1PTt3LacWeyRMRbJBdTANNNHi7hKi9",
    "T00000LXikBV6azoHepngkPUreTAF9C2KbaR9WDC",
    "T00000LZngRU292k9WrnDRc1aC5W4W6bRybG6q6o",
    "T00000LSDwXj1tQz8fwhtcuMBeJcSWhq4EwpnRb6",
    "T00000LdJLdQfvbu5VbNTnWmj6fGgnQRb41BewJ9",
    "T00000LQMVyiWu7SDEv9NxowY9f3nw1VYMPHpLZ9",
    "T00000Lan3mZMJMr8QozzstyJwh746hh1HyZgZ6b",
    "T00000LZgbeaMvw6pQfVURTHCqZXs4yE449xNuQv",
    "T00000LPYua7H2tDzfPwToWZgucRyWC8ha54yVW3",
    "T00000LYRJNisyM2UusRfupeJdm4d4j8EUCkvSR7",
    "T00000LMSaf7EBwcbCVZuHRszuskZSV47B8kPjTU",
    "T00000LZEVKFQEpwdV3vRnCCEiojobs2o3gLActN",
    "T00000LKw29CsqZTpJ1dzwXcd6KqtdbzB1Faeeqm",
    "T00000LXV9n2Es1SEtiU8da6CW6ZQKzxZMvfUbTe",
    "T00000LKpLLDqTfWsXpoSmGKUJPDAm7Tf6gyLWWW",
    "T00000LRC241wjoCApnGJNLQTFehpwmsfjad9h3Y",
    "T00000LQ6zNSWri8tbk1SDcGPCivMjRt5vpcsD4v",
    "T00000LgioHKoUAcs9nTuzJGmoYcb95HotAje84V",
    "T00000LPNvj4WaqHkmFj9jgHTWxeH81ePQ1hNyLZ",
    "T00000Leu4DbWU2C8618YBoJryNV54s4Jc8zS4XH",
    "T00000LeJdaGdHpnnuhtPqavdowT1miKLhBk9rko",
    "T00000LPuMDBfNinurmgQzDS4u9AoNYQf6nrVeUU",
    "T00000LQ5fbkrE4paYzWVQeT34C9D68viWHjMRZf",
    "T00000LavUEEfkyBMbK985m3TN8sc8T8WewK4EQx",
    "T00000LUSBhKmU65nQoH9C3KHv9koEidfddbjgMs",
    "T00000LXRHai81XSJ5Dh7h9ngZHpNVDib6ZugAxB",
    "T00000LMxUb6TJesghQoxqjkBtzazbjhJWFsDGYo",
    "T00000LfaEbH4ehvYx1AaspoTVeaLy7jCPRLue7A",
    "T00000LeCDijRHY2L3DzTq1KrbNMsbyPGyWgrRNK",
    "T00000LKHT9KPs4S9ds7oDe1uUMRs3ndyRFpSTzK",
    "T00000LNBEcv5brjP6a6PicyZmRqxJSsPbtDkvdy",
    "T00000LKmfc34UE97CM5uABSKYpQUniJqky2MKDg",
    "T00000LTM5VG3nL7PQ2UGufQdWvvPobrvdYomLUF",
    "T00000LTTsxCodM4sVrj1oFwXmgHhAmgEjnoQ4KJ",
    "T00000LX5ng1VPD9hsLk7iy7XCa2dnsAhRbudnFE",
    "T00000Lcyy5pWT1x2PissKxAvcJBeBo6Z9rvgzUA",
    "T00000LdfDRPSLWTM1HRLZQWBBY2A9svHPM1sFU2",
    "T00000LbFhbZBRgpbrxteLLz4vixfCaTYF3AeMKN",
    "T00000LRiA3GTebvYkfweGrTuQtJ6Ktqs7qayZXT",
    "T00000LcmAhKuptNrcyPkwjs9Vih6wzapJe9Ssxr",
    "T00000LhsEM6m8CFZuHDtMpaNiUkxzfuFd32Pf6M",
    "T00000LL5uXYeb7Vz3oKkqeeJhh7iZjmKhWJagU4",
    "T00000Leb9rCjPfjzbXbGsU9khbBEoy13r6DSeUc",
    "T00000LTv1p2sRC7PnLcPgFaDpyctQ8b2hyEAcnz",
    "T00000LSMcqiVotVtNzEz6CQbozCMbmcMVSDjiTt",
    "T00000LLnBEMm3whBk1hBwGxVaC2iUSBuzy495aL",
    "T00000LaPaUFP7tsoqtNJocLQe9LypE4z152Didv",
    "T00000LdFj1cZ1Wh284TMN4Yr6uCVKHTBrayXZFr",
    "T00000LMAbjWYaDjuCPYsUuGjRgRA6uKUjPLW57D",
    "T00000LP4qwBsrrKKV4C8SiUMPdiaNKfX8RkH5LY",
    "T00000LfratX4TsWYKFQAfe6gPtcQLk6FKNkrxqR",
    "T00000LgbadWt6KK5daHcjjiE8vh9kM24R8W3y2q",
    "T00000LS17VaZ9G7XuVczwD8zS7gQfRNmCh2yXZs",
    "T00000LZJqD6jJ2nhtEzpX89w1hszS79cZZRund5",
    "T00000LKyu2mtcFonEAxpfyRQGHpKYSzVHy2GgL1",
    "T00000LexmUJaYJs2fuFeKC2sQBJxbJW5gZrj5va",
    "T00000LYQUdoVtCb1DNPtwKFbDESQ45wAh4mkitX",
    "T00000LTmVYXqvzUTyqg2WfrgpmPmAGzL7rePCyZ",
    "T00000LaCoE9rZL9TWtbD8P3u8SakoAScydBevLt",
    "T00000Lfi8zD5VwBhAzuDop35V55MCJVnkzFSZj7",
    "T00000LY1MFrYzau6MboBw7mJpTiDFX7TLovDQok",
    "T00000LfSaKQwvprEXUbCfi3Xwcbh4UXAGLnAqUx",
    "T00000LeqUA6ZR2YR1ZWP2489otdt2vFCPjsM74L",
    "T00000LNDUaDGEBp1xDy3tRPRWSrvnT67idy1tzH",
    "T00000LZJrHTjtapBbWmPjE5kaYrytAkstjq2yRN",
    "T00000LeqRwUrqK98X5WYdCaj9Luymuq7rrsuAYz",
    "T00000LdLfi9AVpSn1mNnqDXRJueiuK1RTFqtcg5",
    "T00000LZMxojqEVZtZMB53tyL5dL9oAWxYZDaeyR",
    "T00000Ld64vZVmCCLt67RG3NoSFVu5quLrd4Jzi2",
    "T00000LbLcsJHU7ZidMiU1fysvNRWyna14iDFxKq",
    "T00000LR27pwMMUdkNgPyjnQTQki2KLpwkp18Yds",
    "T00000LeatMrz1gnP71QY7ePM1TjuUULGQbmePVF",
    "T00000LMmiUprN6HffHP1HeMu1CdV9tWd4BzJaAm",
    "T00000LYYAfmoioBnXazR7qrVBWq2bNkSZdnAJJb",
    "T00000LcWN4JGY54h8h2NhJupTjdm5NJxroqF4YS",
    "T00000Lc7sM3XXyRvJcRKhhMn7zhzDUW4BBcmLFi",
    "T00000LYDoKVZ1vcTRAHX9zyAMRboiaY46NtnKML",
    "T00000LKKnC3cKopBXn4gbaqTVCfTR2HAdjoB6Kn",
    "T00000LYdyE2xyvwGCaqu3S7U6iLUAEKTNNuikVG",
    "T00000LTbu52SN3S1bTw459HX4iALTWdkw7FvjZB",
    "T00000LQgoULGZp6YAGwP76jMH3StPjmhvLykxWV",
    "T00000LUQrpFaRAv3M7euHX6rfFApJrvRDMykENU",
    "T00000LSZkQcFGYbjcc5p94kSPpzP62KrigergFz",
    "T00000LQJNW9GXTNBvWsQDiyUbXfjVPYAGNtH81Y",
    "T00000LPbW7Pn6hF3T8ujK5MgJcXZs7wLfAXjLad",
    "T00000LQ17hzzNAP9FkxmAupDYzqz4BktENyHGt1",
    "T00000LVMkBSh6hxHZE35G7fpdWDF8u5yeRuHkKb",
    "T00000LNqt4j8ve2yPJTJ8GGqTxfuZFuZXcuBegj",
    "T00000LZTr5Xf76wvmnD9HNeJDpBQRd8cR2udex8",
    "T00000LLRNGo1PtQsXs5wfmMaTzrkbZXKmMu4RgJ",
    "T00000LZKaWSh7QSrMCQ6gb2crN2LyijuD4NPvMx",
    "T00000LNY2XEADoaEUGQyKBkC9qanwHbjkihN4fe",
    "T00000Lefj9Nf8Z6fzavorGJhqt4R3izDoLVeVRJ",
    "T00000LTsAUTsB6mBSEzg53qXopa5caQPnydNEiH",
    "T00000LM4JHV4gEHyo6KtNRiqQeik36AqRmW3t24",
    "T00000LbgJouVrboeyPoUg3duVxsndg3ETrWNtSX",
    "T00000Li44D3UyoV6f2VhsUSZQ7hWRyTpPFm2ouG",
    "T00000LNvFy6TyPfsKAmjaDBMKUgYBmdrnSDdg8h",
    "T00000LTwUZ1tBz2obxgXT3CoFCxU5yiFu9LmARD",
    "T00000LWiAzHyPCEsxr9U7GyFew3pud1Pc2A9VMr",
    "T00000LbarLwR3GFvcn5M8PbKLoMgZp1yJzkGhdP",
    "T00000LLJReFnvbM7aKyoh3vwdH3RdkyAHXJWurw",
    "T00000LaKhXPKwuTYVEG2Df4i9WyNLEBjw7VQs6p",
    "T00000LeVgzWgRr4ZHsKEpWuWHyH24mzsvcJpR1j",
    "T00000LgnCuJ1gvpgL3tc9AUMZNFKC3dQMmRH9rH",
    "T00000LRr6eSPgFJwwVssjVkCKwsRKf8YhRrU82C",
    "T00000LXhz9HJcJjhstbT9GkGkLrA1tMKhzmbAnB",
    "T00000LKNWRAhUzCafqeUXXgsxcZ4qzXcoK6ZsM5",
    "T00000Le9peGQfNLiuHuEfENzfMWdDxSFwpWdnEf",
    "T00000LW57R1JZx7NHaLw3An9aDN2DS9AeydhiPt",
    "T00000LSmtLBcvHH2qTm7soyE7DvyCs7A2XU5XbK",
    "T00000LddCFPHKUMWWNGjZxC9Y4RUzMuAWfrJc2k",
    "T00000LXqHmnaCtu5bkeYseyvvv6wG8oN6SMFDQU",
    "T00000LMEregHSiLYSVbu95hCEnvEfSsf5faZmHz",
    "T00000Le6rve5ZT7cvjRJwKK5KCkUTzJQtW4BT1f",
    "T00000LajmTkWE9JZiFfFXoLax647HuUNV4pfD4o",
    "T00000LL1dSgyfuhTWBk4PqpTnnwuYSUZQ96ti1j",
    "T00000LXc7U9KDwHJtWXaQa7X1W3iXkR4LWvkrFn",
    "T00000LXj8c7G3NsDFxTwXo2pVg1KBFXyqmChn4r",
    "T00000LPzAXCFyKdsRHq4aGFC8CXQooZ7Quk9LxN",
    "T00000LPvrfkRYqW8Y2GxQCGw5VoHLUs4CU8tYeJ",
    "T00000LRdBFpyaUQePwSLXunPB5ZchS58qagvEQQ",
    "T00000LLeXsYYdABm4ZWU3ECVF7qryQ3aMxCfPCF",
    "T00000LWCrmgavt1fgPqkUMYdU2PH47vi2JRjZDq",
    "T00000LSLaVTHXcqbeJuZXDn5gnby8AKPHvchGr4",
    "T00000LP16U3GsGDjsnEkT5TFrLe4kLn9uQeYix8",
    "T00000LNGzRb86B1uvitvsRwUJyXd3YzpE4oLYqn",
    "T00000LL8q9Y3WfjHAg33CDKHZ4gcbwiCFdoKhm5",
    "T00000Lg89y1b27auWXTL5VX1zzbQrcwEtp66bCL",
    "T00000LTHJBruUDHM3sXsSxdLQkdvwr9L5284yEt",
    "T00000LYAV3KdnbDyqUsj34t1qcvz1e63vpu9XGC",
    "T00000LS4ZVhRjH6Z8HjsnZNXL7cwAPhDMgHRo22",
    "T00000LRnsxaJisnyJSQRSwxAvhq7cyH4tZV1gLd",
    "T00000LhJx8FiWc2Vyek5NmoNn8sJNrarMCqXLyn",
    "T00000LT1SZnsFSE4L3zrghxVbFErMAX7wcgP927",
    "T00000LTcF2PsxqdDqJoZ3xnwFmF5GLKTY3J6HgY",
    "T00000La77EyotMEsP3nHVVbnWrMeBsCvmogCMpj",
    "T00000LVTtf7D11YDjz8nBsRkWzbLsk8SWfYn1Xn",
    "T00000Lg8BhEbuDrzSrJMTqQAa1ypCAdB5hPrWSi",
    "T00000LRVQYa3xkZaXDebRz1GYGCJR2aKRaeMg7T",
    "T00000LKJK3vS7NNmKct6MxKhqwQP3e9bNtg1VVM",
    "T00000LLAauhiuijnpNzpguDVU1ZZdGLo157hvK7",
    "T00000LSGvkZgecnQbeePUTLVWGmgvfJQfZi7XdH",
    "T00000LMiLNRdM89pjHEsNzAB2KhViir2n8ANmwN",
    "T00000Lcbdf1z36V87Vz2M8zUaqPVjr2Kfnv9mvy",
    "T00000LYDroPHKkhemsgPVUfKqPd3sLkLEumYaip",
    "T00000LRwfioThdMQrhzHqA3grsymwz7yStqRHQ5",
    "T00000LVeAhsUdh2T1RTNcvXfZCZVjykuyVqGQZp",
    "T00000Ldb6X75659EPnZducFyrgV4rGbN595KjuX",
    "T00000LWvSSCeysARzqQ91g3tFTmDzZeSynSdJUA",
    "T00000LNzZMPo3u52F3EEiZZ7vFWiAeVmN3yErSD",
    "T00000LLbSqxBLtkusWUxPLKQQYqzxaghoZW2qoH",
    "T00000LiNUvhQ55wjqSc2YFppNzSNjmM1vvAd6P6",
    "T00000LW3vbZoeg6uzXo3tbnwAW9BwecCrh72EU3",
    "T00000LNfbttzCPUF2RiUaLjB7bPKob4P8ZnTTF1",
    "T00000LLDEbSkXC4YCxcxRzKbm5wTYxXxyELtfgG",
    "T00000LZnqCwGGCaSTQX76gQccdiRQDo5hd1BucD",
    "T00000La8Ttrrv2tDZKgkGHdeDHbbGUKgLuvKYbw",
    "T00000LiJaRvix1k7zEFJwjdc2hsE2QFjQ8tPMav",
    "T00000LPjCSvN3gngJDbu4DnsUPefrAWoybi4PPf",
    "T00000LPhTHgHWm573DWJ8uNBy7tgHcFw67Synmz",
    "T00000LXwFPd7cmrEid5NMxysDpPxC6iqVYWNxxS",
    "T00000LgTqC9DGq2J6ph5hGM8LcVY1D6X4UhPQoF",
    "T00000LUNNP8Fg5FUmCcMhr7kfrWM63a1PzdLzrB",
    "T00000LhPEa25QoFBPpcha3hTPA1MJDKEaSWAZs4",
    "T00000LPdwNn9VaYU83rWPwdBogwNkXpWtp5oqUQ",
    "T00000LQayjAiq6fZFz7XRdFmVz1L72WgAyyLrA5",
    "T00000LQzpmZ4kiRg9AajGh8F2LwHAUas3DiUVET",
    "T00000LgpuPZxsYBWQKML3vfmoZMh8DadJSmSyuJ",
    "T00000LV7hTgxgSyuU4UdnHfUZf4YPBkMUaoghDc",
    "T00000LRHaLCHewiWpMeCEpTrwYUz3UqbXbaSLrd",
    "T00000Lew75b29MeoujDBbyF1J4K65qiiGhY8atJ",
    "T00000LMBqu6nRYiawo3b14MfxAqiX7G15StRXkU",
    "T00000LbBoKu1HgKgt9YGZjPdLmURi6sX5e4ru3E",
    "T00000LeE1bUx9H4am2KWwfXnk3YHiSwVzCHYH4J",
    "T00000LLa97ahY53LaZk5Nm8hDEaHdurua5gvux7",
    "T00000LPy6syvNUeZectLCnET2pQ5LxYZEgct8kF",
    "T00000Li4cx8v7VWTzM2XHTPKUaNZ9L3S21RuZZy",
    "T00000LVdrEvrjU1RvHHK29KieerAYuxjS4V6SCt",
    "T00000LaKo3nyzNLvDNjFjyrW665VyiGtx5oZ9c6",
    "T00000LV4DAa49DDqtbeZA7sS8Zvxh6N6k7Ejpno",
    "T00000LTt4HLQdMKv2kV4wHVaguAMwQqLcS6pFWT",
    "T00000LPZQMcTrQNk4h4V8ocWX6fCKEg8MX47TeS",
    "T00000LZyvLZRR7RVdagJFzMwz2SpU1xRqbTYGcu",
    "T00000LZ7Tto5xhvh1SFfQP1NxBT27H4wTjyaTSg",
    "T00000LQmMEhtKCq5xXPgwuxEqB52ubje1yTiEfX",
    "T00000LSWHuQM8sM48ThbsG8P4Xu9zFZYqiHDQUM",
    "T00000LhGa3oemqbwRbyx8QrexhHBYqYrLZjhXzd",
    "T00000LN7vub6CwRQTNaM4psEreqNoLr4V1GHV74",
    "T00000La8iDpvaAKjQdTZmbDvGUQmTWq1Z4j2qjU",
    "T00000LYf5uu5WEG92XdNa7bWHDKy8hmedeybiqi",
    "T00000LQjKsKs8SAvDi8hZKC13tvfgZSq9pAekZD",
    "T00000LhPqChR1WSKpGLfCrFe243wkk6jph6coGG",
    "T00000LUXazNHvfuwTn9Cfrsc7KewCBBymQNMFuw",
    "T00000LXdKpAZdvZm7uXgFdWgaBM6JFD2CUcMhyn",
    "T00000LbASLAbkcQNmrkt8exbEyRnDBcYDDjMs9m",
    "T00000Lav7o92pNkKe3kcsneYth2uMJn8BdkeT48",
    "T00000LUctgNQqqf2wMggDu9aKdVjkX4J6YMqF12",
    "T00000LUH695VeXpqjRH6zGQbJud3NyvmVSudtnJ",
    "T00000LL94wt5vbg6XjWayv2WHFRNpvxLUa4x7tT",
    "T00000Lgx4p4KfqPEqkP1BLFKw6xp4j6DKQ9Rq11",
    "T00000LQALQWt5vq1MTJeyjDydf8TiEbVweRrt4u",
    "T00000LLUsmingYs1GD55swwrHVgiwih2jmGNZEA",
    "T00000LNFYEapqMnPQMA3SSdoEF147sHX2ygyUMQ",
    "T00000LStaptkRnGe737a5mKGwrYhTrztBieWE3z",
    "T00000Lat5fv5oh82oZ27YVPyxv5CaBBetqiAdUD",
    "T00000LhDuyRsUBG3K447a3EJK37jszEU4MToZfg",
    "T00000LZXsZMMo2Ysi98rEBHjLyubPQKy6KUBHpY",
};

class xtop_test_elect : public xvm::system_contracts::xelect_consensus_group_contract_t {
    using xbase_t = xvm::system_contracts::xelect_consensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_elect);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_elect);

    explicit xtop_test_elect(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}
    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }
};
using xtest_elect_t = xtop_test_elect;

class xtop_test_reward_algorithm : public testing::Test {
public:
    xtop_test_reward_algorithm() {
        for (size_t i = 0; i < test_accounts.size(); i++) {
            m_test_accounts.insert(std::make_pair(test_accounts[i], i));
        }
    }
    ~xtop_test_reward_algorithm() {}

    void set_group_size(uint32_t min, uint32_t max) {
        top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(min));
        top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(max));
    }

    std::string rand_str(int length) {
        char tmp;
        std::string buffer;
        std::random_device rd;
        std::default_random_engine random(rd());
        for (auto i = 0; i < length; i++) {
            tmp = random() % 36;
            if (tmp < 10) {
                tmp += '0';
            } else {
                tmp -= 10;
                tmp += 'a';
            }
            buffer += tmp;
        }
        return buffer;
    }

    int rand_num(int min, int max) {
        char tmp;
        std::string buffer;
        std::random_device rd;
        std::default_random_engine random(rd());
        int temp = max - min + 1;
        return (random() % temp) + min;
    }

    void test_stake_reward_v1(bool has_tx_workload, uint32_t account_num = 32, uint32_t test_days = 10) {
        set_group_size(account_num, account_num);
        // make map nodes
        std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> map_nodes;
        for (size_t i = 0; i < account_num; i++) {
            data::system_contract::xreg_node_info node;
            node.m_account = common::xaccount_address_t{test_accounts[i]};
            node.miner_type(common::xminer_type_t::advance);
            node.m_vote_amount = 10;
            node.m_account_mortgage = 10*TOP_UNIT; 
            map_nodes.insert({node.m_account, node});
        }
        // stake
        std::vector<uint32_t> stake;
        for (size_t i = 0; i < account_num; i++) {
            stake.emplace_back((100 + i*10) * 10000);
        }
        // make elect
        common::xgroup_address_t group_addr{m_nid, m_zid, m_cid, m_gid};
        xunit_service::xelection_cache_face::elect_set elect_set;
        for (size_t i = 0; i < account_num; i++) {
            common::xaccount_election_address_t account_election_addr{common::xaccount_address_t{test_accounts[i]}, common::xslot_id_t(i)};
            common::xnode_address_t node_addr{group_addr, account_election_addr};
            // xunit_service::xelection_cache_face::xelect_data elect_data;
            // elect_data.xip = node_addr.xip2().value();
            // elect_data.joined_version = common::xelection_round_t{0};
            // elect_data.staking = stake[i];
            top::data::xnode_info_t node_info;
            node_info.address = node_addr;
            node_info.election_info.comprehensive_stake(stake[i]);
            node_info.election_info.joined_epoch(common::xelection_round_t{0});
            node_info.election_info.raw_credit_score(1000000);
            elect_set.emplace_back(node_info);
            // std::cout << "xip: " << node_addr.xip2().value().high_addr << " " << node_addr.xip2().value().low_addr << std::endl;
        }
        // caculate reward
        uint64_t viewid = 0;
        uint64_t time_height = 0;
        const uint32_t table_blocks_per_12h = 90;
        const uint32_t table_num = 32;
        std::map<std::string, ::uint128_t> reward;
        std::vector<uint64_t> workload(account_num, 0);
        data::system_contract::xaccumulated_reward_record reward_record;
        for (size_t days = 0; days < test_days; days++) {
        // for (size_t days = 0; days < 1; days++) {
            for (size_t count = 0; count < 2; count++) {
                std::vector<uint64_t> workloads(account_num, 0);
                for (size_t i = 0; i < table_blocks_per_12h; i++) {
                // for (size_t i = 0; i < 1; i++) {
                    for (size_t j = 0; j < table_num; j++) {
                    // for (size_t j = 0; j < 1; j++) {
                        std::string table(std::string(sys_contract_sharding_workload_addr) + "@" + std::to_string(j));
                        uint64_t random = viewid + base::xvaccount_t::get_xid_from_account(table);
                        random = base::xhash64_t::digest(base::xstring_utl::to_hex(rand_str(64))) + random;
                        std::vector<common::xfts_merkle_tree_t<xvip2_t>::value_type> candidates;
                        // std:cout << "random: " << random << std::endl;
                        for (auto iter = elect_set.begin(); iter != elect_set.end(); iter++) {
                            candidates.push_back({static_cast<common::xstake_t>(iter->election_info.comprehensive_stake() + 1), static_cast<xvip2_t>(iter->address.xip2())});
                            // std::cout << "xip: " << iter->xip.high_addr << " " << iter->xip.low_addr << std::endl;
                        }
                        std::vector<common::xfts_merkle_tree_t<xvip2_t>::value_type> leaders;
                        leaders = common::select<xvip2_t>(candidates, random, 1);
                        auto slotid = get_node_id_from_xip2(leaders[0].second);
                        // std::cout << slotid << std::endl;
                        workloads[slotid] += XGET_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tableblock);
                        if (has_tx_workload) {
                            workloads[slotid] += rand_num(1, 5);
                            // std::cout << rand_num(1, 5) << std::endl;
                        }
                    }
                    viewid = viewid + 1 + 48 / 3; // 8min align to 3
                }
                for (size_t i = 0; i < account_num; i++) {
                    workload[i] += workloads[i];
                }
                // for (auto i : workloads) {
                //     std::cout << i << std::endl;
                // }
                // caculate reward
                time_height += 4320;
                ::uint128_t total_issuance = m_reward_contract.calc_total_issuance(
                    time_height, XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward), XGET_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio), reward_record);
                ::uint128_t auditor_total_workload_rewards = total_issuance * XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio) / 100;
                // make workload
                std::map<common::xcluster_address_t, data::system_contract::xgroup_workload_t> auditor_workloads_detail;
                data::system_contract::xgroup_workload_t workload;
                workload.group_address_str = group_addr.to_string();
                for (size_t i = 0; i < account_num; i++) {
                    if (workload.m_leader_count.count(test_accounts[i])) {
                        workload.m_leader_count[test_accounts[i]] += workloads[i];
                    } else {
                        workload.m_leader_count[test_accounts[i]] = workloads[i];
                    }
                    workload.group_total_workload += workloads[i];
                }
                auditor_workloads_detail.insert(std::make_pair(group_addr, workload));
                // caculate reward
                for (auto const & node : map_nodes) {
                    ::uint128_t reward_to_self = 0;
                    m_reward_contract.calc_auditor_workload_rewards(node.second, {account_num, account_num, 0}, auditor_workloads_detail, auditor_total_workload_rewards, reward_to_self);
                    if (reward.count(node.first.to_string())) {
                        reward[node.first.to_string()] += reward_to_self;
                    } else {
                        reward[node.first.to_string()] = reward_to_self;
                    }
                }
            }
        }
        ::uint128_t total_reward = 0;
        for (size_t i = 0; i < account_num; i++) {
            std::cout << test_accounts[i] << "  stake: " << stake[i] << ",  workload: " << workload[i] << ",  reward: " << static_cast<uint64_t>(reward[test_accounts[i]] / data::system_contract::REWARD_PRECISION) << '.'
                      << static_cast<uint32_t>(reward[test_accounts[i]] % data::system_contract::REWARD_PRECISION) << std::endl;
            total_reward += reward[test_accounts[i]];
        }
        data::system_contract::xaccumulated_reward_record calc;
        ::uint128_t total_issuance = m_reward_contract.calc_total_issuance(
                8640 * 10, XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward), XGET_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio), calc);
        total_issuance =  total_issuance * XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio) / 100;
        std::cout << "should reward: " << static_cast<uint64_t>(total_issuance / data::system_contract::REWARD_PRECISION) << '.'
                  << static_cast<uint32_t>(total_issuance % data::system_contract::REWARD_PRECISION)
                  << std::endl;
        std::cout << "total reward: " << static_cast<uint64_t>(total_reward / data::system_contract::REWARD_PRECISION) << '.'
                  << static_cast<uint32_t>(total_reward % data::system_contract::REWARD_PRECISION)
                  << std::endl;
    }

    void test_stake_reward_v2(uint32_t account_num = 32, uint32_t test_days = 10) {
        set_group_size(account_num, account_num);
        // make map nodes
        std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> map_nodes;
        for (size_t i = 0; i < account_num; i++) {
            data::system_contract::xreg_node_info node;
            node.m_account = common::xaccount_address_t{test_accounts[i]};
            node.miner_type(common::xminer_type_t::advance);
            node.m_vote_amount = 10;
            node.m_account_mortgage = 10*TOP_UNIT; 
            map_nodes.insert({node.m_account, node});
        }
        // stake
        std::vector<uint32_t> stake;
        for (size_t i = 0; i < account_num; i++) {
            stake.emplace_back((100 + i*10) * 10000);
        }
        // make elect
        common::xgroup_address_t group_addr{m_nid, m_zid, m_cid, m_gid};
        xunit_service::xelection_cache_face::elect_set elect_set;
        for (size_t i = 0; i < account_num; i++) {
            common::xaccount_election_address_t account_election_addr{common::xaccount_address_t{test_accounts[i]}, common::xslot_id_t(i)};
            common::xnode_address_t node_addr{group_addr, account_election_addr};
            // xunit_service::xelection_cache_face::xelect_data elect_data;
            // elect_data.xip = node_addr.xip2().value();
            // elect_data.joined_version = common::xelection_round_t{0};
            // elect_data.staking = stake[i];
            top::data::xnode_info_t node_info;
            node_info.address = node_addr;
            node_info.election_info.comprehensive_stake(stake[i]);
            node_info.election_info.joined_epoch(common::xelection_round_t{0});
            node_info.election_info.raw_credit_score(1000000);
            elect_set.emplace_back(node_info);
            // std::cout << "xip: " << node_addr.xip2().value().high_addr << " " << node_addr.xip2().value().low_addr << std::endl;
        }
        ::uint128_t total_reserve = static_cast<::uint128_t>(data::system_contract::TOTAL_RESERVE) * data::system_contract::REWARD_PRECISION *
                                        XGET_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio) / 100;
        ::uint128_t reward_unit = total_reserve / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR;
        const uint32_t table_num = 32;
        const uint64_t total_time_height = test_days * 24 * 360;
        std::vector<uint64_t> table_time_last(table_num, 0);
        std::vector<uint64_t> table_reward_last(table_num, 0);
        std::map<std::string, ::uint128_t> reward;
        for (size_t i = 0; i < table_num; i++) {
            int viewid = 0;
            uint64_t time_height_left = total_time_height;
            while (time_height_left > 0) {
                uint64_t time_this = rand_num(4000, 7000);
                if (time_this > time_height_left) {
                    time_this = time_height_left;
                }
                uint64_t time_last = table_time_last[i];
                uint64_t reward_last = table_reward_last[i];
                uint64_t reward_this = 0;
                if (time_last == 0 && reward_last == 0) {
                    reward_this = reward_unit * time_this * XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio) / 100 / 64;
                } else {
                    reward_this = reward_unit * (time_this + time_last) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio) / 100 / 64 - reward_last;
                }
                
                uint64_t reward_light = reward_this / 128;
                table_time_last[i] = time_this;
                table_reward_last[i] = reward_this;
                time_height_left -= time_this;
                std::string table{std::string(sys_contract_sharding_workload_addr) + "@" + std::to_string(i)};
                for (size_t j = 0; j < 128; j++) {
                    uint64_t random = viewid + base::xvaccount_t::get_xid_from_account(table);
                    random = base::xhash64_t::digest(base::xstring_utl::to_hex(rand_str(64))) + random;
                    std::vector<common::xfts_merkle_tree_t<xvip2_t>::value_type> candidates;
                    for (auto iter = elect_set.begin(); iter != elect_set.end(); iter++) {
                        candidates.push_back({static_cast<common::xstake_t>(iter->election_info.comprehensive_stake() + 1 + 1), static_cast<xvip2_t>(iter->address.xip2())});
                        // std::cout << "xip: " << iter->xip.high_addr << " " << iter->xip.low_addr << std::endl;
                    }
                    std::vector<common::xfts_merkle_tree_t<xvip2_t>::value_type> leaders;
                    leaders = common::select<xvip2_t>(candidates, random, 1);
                    auto slotid = get_node_id_from_xip2(leaders[0].second);
                    if (reward.count(test_accounts[slotid])) {
                        reward[test_accounts[slotid]] += reward_light;
                    } else {
                        reward[test_accounts[slotid]] = reward_light;
                    }
                    viewid = viewid + 1 + time_this / 128 / 3; // 8min align to 3
                }
            }
        }
        ::uint128_t total_reward = 0;
        for (size_t i = 0; i < account_num; i++) {
            std::cout << test_accounts[i] << "  stake: " << stake[i] << ",  reward: " << static_cast<uint64_t>(reward[test_accounts[i]] / data::system_contract::REWARD_PRECISION)
                      << '.' << static_cast<uint32_t>(reward[test_accounts[i]] % data::system_contract::REWARD_PRECISION) << std::endl;
            total_reward += reward[test_accounts[i]];
        }
        data::system_contract::xaccumulated_reward_record calc;
        ::uint128_t total_issuance = m_reward_contract.calc_total_issuance(
                8640 * 10, XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward), XGET_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio), calc);
        total_issuance =  total_issuance * XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio) / 100 / 2;
        std::cout << "should reward: " << static_cast<uint64_t>(total_issuance / data::system_contract::REWARD_PRECISION) << '.'
                  << static_cast<uint32_t>(total_issuance % data::system_contract::REWARD_PRECISION)
                    << std::endl;
        std::cout << "total reward: " << static_cast<uint64_t>(total_reward / data::system_contract::REWARD_PRECISION) << '.'
                  << static_cast<uint32_t>(total_reward % data::system_contract::REWARD_PRECISION)
                    << std::endl;
    }

    static void SetUpTestCase() {
        top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(DEFAULT_MIN_GROUP_SIZE));
        top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(DEFAULT_MAX_GROUP_SIZE));
        top::config::config_register.get_instance().set(config::xauditor_nodes_per_segment_onchain_goverance_parameter_t::name, std::to_string(27));
        top::config::config_register.get_instance().set(config::xworkload_per_tableblock_onchain_goverance_parameter_t::name, std::to_string(2));
        top::config::config_register.get_instance().set(config::xmin_ratio_annual_total_reward_onchain_goverance_parameter_t::name, std::to_string(2));
        top::config::config_register.get_instance().set(config::xadditional_issue_year_ratio_onchain_goverance_parameter_t::name, std::to_string(8));
        top::config::config_register.get_instance().set(config::xauditor_reward_ratio_onchain_goverance_parameter_t::name, std::to_string(10));
    }
    static void TearDownTestCase() {}

    std::map<std::string, uint32_t> m_test_accounts;
    uint32_t m_test_accounts_num{test_accounts.size()};
    common::xnode_type_t m_node_type{common::xnode_type_t::consensus_auditor};
    common::xnetwork_id_t m_nid{common::xbeacon_network_id};
    common::xzone_id_t m_zid{common::xconsensus_zone_id};
    common::xcluster_id_t m_cid{common::xdefault_cluster_id};
    common::xgroup_id_t m_gid{common::xauditor_group_id_begin};
    xtop_test_elect m_elect_consensus_group{common::xbeacon_network_id};
    xstake::xzec_reward_contract m_reward_contract{common::xbeacon_network_id};
};
using xtest_reward_algorithm_t = xtop_test_reward_algorithm;

#if 0
TEST_F(xtest_reward_algorithm_t, test_static_group_workload_reward1) {
    std::cout << "test_static_group_workload_reward:" << std::endl;
    std::cout << "1. test only one \"static\" auditor group with 32 accounts whose stakes are different;" << std::endl;
    std::cout << "2. test time: 10 days;" << std::endl;
    std::cout << "3. calculate workload only with block workload(no tx workload);" << std::endl;
    std::cout << "4. 1tps 1h 7.5 blocks per table, total 5760 blocks per day(7.5 * 16 tables * 24 hours * 2 shards);" << std::endl;
    std::cout << "5. reward interval: 12 hours;" << std::endl;
    std::cout << "6. expect: more stake, more workload and reward;" << std::endl;
    std::cout << "result:" << std::endl;
    test_stake_reward_v1(false);
}

TEST_F(xtest_reward_algorithm_t, test_static_group_workload_reward2) {
    std::cout << "test_static_group_workload_reward:" << std::endl;
    std::cout << "1. test only one \"static\" auditor group with 32 accounts whose stakes are different;" << std::endl;
    std::cout << "2. test time: 10 days;" << std::endl;
    std::cout << "3. calculate workload with block workload and tx workload);" << std::endl;
    std::cout << "4. 1tps 1h 7.5 blocks per table, total 5760 blocks per day(7.5 * 16 tables * 24 hours * 2 shards);" << std::endl;
    std::cout << "5. reward interval: 12 hours;" << std::endl;
    std::cout << "6. expect: more stake, more workload and reward;" << std::endl;
    std::cout << "result:" << std::endl;
    test_stake_reward_v1(true, 32, 1);
}

TEST_F(xtest_reward_algorithm_t, test_static_group_workload_reward3) {
    test_stake_reward_v2();
}

#ifdef ELECT_STAKE_TEST
TEST_F(xtest_reward_algorithm_t, test_stake) {
    xrange_t<config::xgroup_size_t> group_size_range{XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size)};
    std::vector<std::uint16_t> elect_in_times{std::vector<std::uint16_t>(test_accounts.size(), 0)};
    data::election::xstandby_network_result_t standby_network_result;
    // make_standby_network();
    for (std::size_t index = 0; index < m_test_accounts_num; index++) {
        common::xnode_id_t node_id;
        auto it = m_test_accounts.begin();
        for (; it != m_test_accounts.end(); it++) {
            if (it->second == index) {
                node_id = common::xnode_id_t{it->first};
                break;
            }
        }
        assert(it != m_test_accounts.end());
        data::election::xstandby_node_info_t standby_node_info;
        // standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.stake_container.insert({common::xnode_type_t::consensus_auditor, (100 + index) * 10000});
        standby_node_info.stake_container.insert({common::xnode_type_t::consensus_validator, (100 + index) * 10000});
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = (m_node_type == common::xnode_type_t::validator) ? common::xminer_type_t::validator : common::xminer_type_t::advance;
#endif
        standby_network_result.result_of(m_node_type).insert({node_id, standby_node_info}).second;
    }

    data::election::xelection_network_result_t election_network_result;
    for (std::size_t index = 1; index <= GROUP_ELECT_COUNT; ++index) {
        common::xlogic_time_t time{index};
        std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        auto random_seed = static_cast<uint64_t>(rng());
        ASSERT_TRUE(m_elect_consensus_group.elect_group(m_zid, m_cid, m_gid, time, time, random_seed, group_size_range, standby_network_result, election_network_result));
        
        auto & election_group_result = election_network_result.result_of(m_node_type).result_of(m_cid).result_of(m_gid);
        std::vector<std::size_t> node_ids;
        for (auto & election_result : election_group_result) {
            auto & election_info = top::get<data::election::xelection_info_bundle_t>(election_result);
            auto node_id_string = election_info.node_id().to_string();
            node_ids.push_back(m_test_accounts[node_id_string]);
        }

        for (auto node_id : node_ids) {
            elect_in_times[node_id]++;
        }
    }

    // print_elect_result();
    auto auditor_nodes_per_segment = XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_nodes_per_segment);
    std::size_t avg = 0;
    for (std::size_t index = 0; index < m_test_accounts_num; index++) {
        std::printf("%5d ", elect_in_times[m_test_accounts_num - 1 - index]);
        avg += elect_in_times[m_test_accounts_num - index];
        if ((index + 1) % auditor_nodes_per_segment == 0) {
            std::printf("avg: %5zu \n", avg / auditor_nodes_per_segment);
            avg = 0;
        }
    }
    std::printf("\n");
}

TEST_F(xtest_reward_algorithm_t, test_rand_str) {
    for (auto i = 0; i < 32; i++) {
        std::cout << rand_str(64) << std::endl;
    }
}
#endif
#endif

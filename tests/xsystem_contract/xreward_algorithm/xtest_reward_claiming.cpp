// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <sstream>

#define private public

#include "xconfig/xconfig_register.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xstore/xaccount_context.h"
#include "xvm/xsystem_contracts/xreward/xtable_reward_claiming_contract.h"

using namespace top;
using namespace std;
static std::vector<string> voters = {
    "T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc",
    "T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW",
    "T00000LhLPkC9q7BfcjPpwWcb4pgZ3fHqTxdUywi",
    "T00000LWfXZgVPa8mmuDhxQjAtSgVaB9exKsQBWE",
    "T00000LNomDyH8kN2zQhexGbg4dMJY1ZbAAekZ8Y",
    "T00000LcYvxYB3EAnPcBo9hPybrDM2ZB5v5JFcyg",
    "T00000LZhrRcNKoisFNYcRk8yRkkBuT3S2XdqCWt",
    "T00000LTQPcfP2MnUstYLm2s2ABCRCjxSeL4NPmx",
};

static std::vector<string> advs = {
    "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD", "T00000LM3v9AxCW2ek3hWM2XYe77BnGTyN3wjjnc", "T00000LV5bD34qXGux3c3qwVw4RBRCP6RdNsGt5H", "T00000LZEmHiD2LZaEAF9eAFoskB6BowtPUhKZSc",
    "T00000LNmcQCfjjk1Dj691L75egthyzyBAdkRtx4", "T00000LaYJvA3drXAhjzmke4dmS1WAAUKxXraAua", "T00000LMuaSSzFVvrktEZhuP64tZjY5nkKrAMhW3", "T00000LeqRDKSdLtWBiMXuJp615d1esfswuTUhwi",
    "T00000LWD9azGw3EP8neeHjNyVxH3EL69oEza8ms", "T00000LR2wTU2H7jVCWxFph5oWbaNZJwmWKx3Ef8", "T00000LgrChMEn7uYbzgQjn4gVgdzg2hse4huCoH", "T00000LfjqY18epcoddktGM2uCdTtyNtfe8pJ1QC",
    "T00000LLzbhskFTjShEk9FHSgeuyW9VyBonkKL15", "T00000LdbJ89sVheXwtoM13UCG3ufuYfrK4wzEUo", "T00000Lh3TbVKYCgjMWuBPZGxsJCHtJx6wiDhcdB", "T00000LfubbRqcxBhtzg1hFJQKqX5K22PX9opvpu",
    "T00000LQRemCBd4nFK7d2qFu5nx9Mx4xxYMKZqQV", "T00000LKph1D2S5ENsLy1nYgdJkZuL3oPrk1Bsmc", "T00000LKktoA7i2632q1uppi81DcYYAky36PCryN", "T00000LPvFYq8fayare1kf2KoUY4sZHPmgnDvLK7",
    "T00000LZPvkgLukkLUCAXqB7kKGpynqTbpBK4Giy", "T00000LMx5PFtPqtehptpKkX9KiLWYSDTKrUWdV9", "T00000Lfg4DdD4vgcgB5FpLoHcYC54YoRtN1fFmU", "T00000Lccfxfbm7wN2b2ZaYMVineDywz9XkzuEkz",
    "T00000Lh4MmMm8oVx9nXbicJLzDHP8bBpm6hKds6", "T00000LNQ28AGSay56YRGyjyPdP37D4u9eM1KT2N", "T00000LdLjByQuB4GQ3YJW16oczF6Uzfk2z93U3m", "T00000LXnKEfteKAJbVi6CeKQ4EDyTRvgvvCJYEh",
    "T00000LPdRp5nEAHs2ubNtfio3JXtzJEEujKMMJQ", "T00000LUwsGb2qukJWrz7nJWHutwnwek8uDgSe2C", "T00000LbHHPogMqHdvjxzLrJvWHvat3Np5hsZ3zC", "T00000LYUST38rkkHeFn7edrYQJWbjhnJDqs58sq",
};

static std::vector<string> validators = {
    "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb", "T800007395208c7681bf66f74711ea41c766cd4bf1561a", "T80000382fc7b70e7c490f19a20f4a99402d51a984674a",
    "T80000a36a6d82fa55894ea36d7b043522b808fef45695", "T800009bd1a948541e82c66d9f408a29aa3d28a8501afc", "T80000cb9443e74cc8d362752c88f589c9d1d12feeb5a9",
    "T8000047af5ada6db181d000a0c90aaa4535e6b78c3804", "T80000a674a507fdd986ad2c40a90b8e9935c440d52893", "T80000bc1769315744f5dd54289f418374f9ef6b6864f5",
    "T800000c65dd5cce9d7f89079c72c37883a72186d4a3fc", "T80000ce1564070a34cea122a9e64d34ce7b19ef48bf8e", "T800007834238ad4f698e6a2689451468fb55e7b563d3b",
    "T80000c2a47a2ccd629b6677e0464ab4f61cfc54120e18", "T80000da9e3fa379cd18f396419968dd8f76fd76650c48", "T8000086ead9e12f1acccf291dc916bd20943f45b18695",
    "T8000019cd5e985a531ad03cfa078b770d33162c7a836b", "T80000421aee1ca67d86a95b7066d564a01208077b080f", "T80000ebfa4f6b30ac91f778a14ac2e2b066d1c347c38a",
    "T8000021318c50c2d830a6f8909d1586ed2a8b03cc4ee3", "T80000671d45285872bf2a2fbde0bc138b80a00a958eb0", "T8000011e5cb4b3ed7bc3b2aa2d70ea9c5c1c79e30d9e5",
    "T800002a7130d1de4984bebc04e5912317a758ebb4ab1d", "T80000265adc6da24906bb809abdaf05bbec8e70681fec", "T8000055364f052a0b56aa324e239df04117608159502a",
    "T80000fc67c750c620e2cca0829a29533f54a3d968e9f5", "T800001360a952acfdc3f8eac6f3733c04e912a423abda", "T80000359bcb3c1a761b373e5217c8047fe75367812e35",
    "T80000088f898caa66399039e4ef8f43dbe1af110a976a", "T80000a914cc9791057bd77d7d6744135c3e8d2a830533", "T80000caec82f6e880d37d8c6478591a0f317e4ae98b87",
    "T800006af5f308d4a9ee3d8f0989dc37d6129a894a20f6", "T800009d4560ba28bdb7d2bce559de6b6ffa676764c241",
};

static const std::vector<string> test_accounts = {
    "T00000Li18Pb8WuxQUphZPfmN6QCWTL9kwWPTns1", "T00000LawC9X7onZ23ftMfsBb8XtCHH7yHPbUWef", "T00000LZTVtSXAAHpw5L92Jw4VkatpX64YW6Sona", "T00000LfoXb1vXXQiPxyQbnHsUgZKrNfTqLj7Rmr",
    "T00000LUgUM5DcTb4Be4gUEwCWByhUf191Qu9nFr", "T00000LPSxv3AVu7Kb4oET1QsVzFDiofZHF2BBH4", "T00000LXtwPC88mTkBo5cSBZ6rFFwifnnvSU2sB3", "T00000LiYuDqviN9Kzi84t2DZhamN7E6tukogyv8",
    "T00000LewNgnEwG8E1FwxiqY6sb7et7B3VzKmr63", "T00000LSbd1EhPLc45TM8FnjFJFbjqUMaoeiiK7L", "T00000LZhN8dgiPu3UXAniFcMGddgsDTu6upfCvw", "T00000LYsCRMporiwbcv4THJzAtWhzpvgK2qysYU",
    "T00000LbJNSqcjeNtAb3oxMoTiv9TxMRK4F5AbBh", "T00000LfAgMFFHuVHixLqXtVhHf97ApjBpcFRiMK", "T00000LhV1uFjJwLCTNFapuLYKMsaiqiYnSX5hoK", "T00000LeCT6tj3eEE7Y1PPubHB95SuxVQt8UEmBu",
    "T00000LadFgEiL8jmeN3gHMgzQccHLDdsPkq4DGP", "T00000LeQftFqhhtPBcWDNYLHSMDjUzi4wZs3wLD", "T00000LQFCSEYeFzXYtLfTPRbm2dYVkRzdDJNu1o", "T00000LeQCRU2BtZ29KZ5SsPvifGsNo8MeoSDQn4",
    "T00000LTmVu99sw7snYC3oaSv5HZqmJ9g9dGD3Xv", "T00000LSrDkb5xT1HPxLC5YsBPsxxJRQUnSdBgAc", "T00000LYNVeHLfZD6TVQuqsGvpzQM5Ywx1yBdPKX", "T00000LLUocKMpyskr1bX1DdtrpiqKvy3oWY5yFW",
    "T00000LcNVKFikgix1SFdbSNru19CMpRcN3ikcjC", "T00000LLxttrYBMCDKCP5aiEqDaXjhBrJ1sV9mSq", "T00000LKRkkE3PanNsN6VWaQ9zG941gYaNLcsXPm", "T00000LNYDgKGbAMpSegkNn7VH1TgmEvHT3bsh9n",
    "T00000Lb3QjHwgP6stckHpWVPoakev3kWAEauhcP", "T00000LSkNiBexHjKqvuuDzX6CQSH2L5MDLZYZw1", "T00000Lc9tqkGpah9GfKfQCJZCByeVCp3BBxpUsG", "T00000LQf6B7usSkDeq1WycX6JoGMVUQfA4Fawto",
    "T00000LSBDQPnVvXUCzJLyvdKd1cPqpCFgZksfth", "T00000LcXFu9yDj3NxYWE2YSfKxeDo7PpigK1c2E", "T00000LUrznL9rbp9owuxfyRBFTsShfwS4BBzJew", "T00000Lg6qi4yT1BY2HjJh1RKcRTXcZN4AcuajWZ",
    "T00000LYL6TuoLyZP9L88nA1G5UTaxhsiZbKVLDK", "T00000LT4vk1Zt1JkAFcUwpvcYhde46dUR2ZeZsx", "T00000LLDRirM4pWLAzfs1SLnD8j25Rx9KWS43KJ", "T00000LevCP3gaHypGbZarCdZUrLCryXvrxM6Taj",
    "T00000LYvSqQkaEcm952A7u16PhPDjNmXpqZ6nW2", "T00000LeX78Rymjnt2LTdjkvMWqWHyBR4PmkVy4k", "T00000LSnwAtBV7bnb5ASLpoCkCMvZrh19JynZSG", "T00000LZp5MtigSGBAfnKoZ5VHYwuY3yz14H1YaY",
    "T00000LckQSSYGvm2nqvBGKR6k7rkcnoZbGHJcRr", "T00000LS4ubZfQAhdMVjqxtAbJxmi6MnY91jqUww", "T00000LVhE9KjZ4cxsQ5dxNQ3NjVQTx9Hr9BiE98", "T00000LhnFCkZ9ShxABVdsdWqkV9ihuJ1Sxim1wz",
    "T00000Lcoi5MBsxPU9GyBeJL29abksoEGewzBACm", "T00000LePGLhYtnNGEmUq4MwitS27SqpoVt1anqe", "T00000LTby2SWekxyzw24gsnJTFMHEf4UBPs58d6", "T00000LhxdfkbVgjr6f8QAXQ59J1JtwSWvRwocc6",
    "T00000LbJfJFoRjsEMyDe47Qdghrb15A94aZMvZw", "T00000LKFafZ8m7afDvotwnGUk1Gazr228K18212", "T00000LfdLLJgZQo455Wc8LBpQUpERLvjhnvsFdB", "T00000LQMRinmFmBaP5P5uJRqVm1XpXdcTByHtEp",
    "T00000LXe7o9fZmhJFBdwRfCkzQ8nhHZDsVs1YqA", "T00000LXFLb3vZvaP97TRFZWZbXcct1nvCHQw4CZ", "T00000LUSBUWnZpE2kAfJKGFQ3xWWbrWPrGd8b61", "T00000LRVy35KjgqhzLSmFTyfZyenPZ1f9VvdYK2",
    "T00000LbTNWrrv2nT3TEdQv3ozNFhF25d7BooFhh", "T00000Lc2DJEqsLbX5a8ftmahTBoBo5vvuDQmpzF", "T00000LMMd1PTt3LacWeyRMRbJBdTANNNHi7hKi9", "T00000LXikBV6azoHepngkPUreTAF9C2KbaR9WDC",
    "T00000LZngRU292k9WrnDRc1aC5W4W6bRybG6q6o", "T00000LSDwXj1tQz8fwhtcuMBeJcSWhq4EwpnRb6", "T00000LdJLdQfvbu5VbNTnWmj6fGgnQRb41BewJ9", "T00000LQMVyiWu7SDEv9NxowY9f3nw1VYMPHpLZ9",
    "T00000Lan3mZMJMr8QozzstyJwh746hh1HyZgZ6b", "T00000LZgbeaMvw6pQfVURTHCqZXs4yE449xNuQv", "T00000LPYua7H2tDzfPwToWZgucRyWC8ha54yVW3", "T00000LYRJNisyM2UusRfupeJdm4d4j8EUCkvSR7",
    "T00000LMSaf7EBwcbCVZuHRszuskZSV47B8kPjTU", "T00000LZEVKFQEpwdV3vRnCCEiojobs2o3gLActN", "T00000LKw29CsqZTpJ1dzwXcd6KqtdbzB1Faeeqm", "T00000LXV9n2Es1SEtiU8da6CW6ZQKzxZMvfUbTe",
    "T00000LKpLLDqTfWsXpoSmGKUJPDAm7Tf6gyLWWW", "T00000LRC241wjoCApnGJNLQTFehpwmsfjad9h3Y", "T00000LQ6zNSWri8tbk1SDcGPCivMjRt5vpcsD4v", "T00000LgioHKoUAcs9nTuzJGmoYcb95HotAje84V",
    "T00000LPNvj4WaqHkmFj9jgHTWxeH81ePQ1hNyLZ", "T00000Leu4DbWU2C8618YBoJryNV54s4Jc8zS4XH", "T00000LeJdaGdHpnnuhtPqavdowT1miKLhBk9rko", "T00000LPuMDBfNinurmgQzDS4u9AoNYQf6nrVeUU",
    "T00000LQ5fbkrE4paYzWVQeT34C9D68viWHjMRZf", "T00000LavUEEfkyBMbK985m3TN8sc8T8WewK4EQx", "T00000LUSBhKmU65nQoH9C3KHv9koEidfddbjgMs", "T00000LXRHai81XSJ5Dh7h9ngZHpNVDib6ZugAxB",
    "T00000LMxUb6TJesghQoxqjkBtzazbjhJWFsDGYo", "T00000LfaEbH4ehvYx1AaspoTVeaLy7jCPRLue7A", "T00000LeCDijRHY2L3DzTq1KrbNMsbyPGyWgrRNK", "T00000LKHT9KPs4S9ds7oDe1uUMRs3ndyRFpSTzK",
    "T00000LNBEcv5brjP6a6PicyZmRqxJSsPbtDkvdy", "T00000LKmfc34UE97CM5uABSKYpQUniJqky2MKDg", "T00000LTM5VG3nL7PQ2UGufQdWvvPobrvdYomLUF", "T00000LTTsxCodM4sVrj1oFwXmgHhAmgEjnoQ4KJ",
    "T00000LX5ng1VPD9hsLk7iy7XCa2dnsAhRbudnFE", "T00000Lcyy5pWT1x2PissKxAvcJBeBo6Z9rvgzUA", "T00000LdfDRPSLWTM1HRLZQWBBY2A9svHPM1sFU2", "T00000LbFhbZBRgpbrxteLLz4vixfCaTYF3AeMKN",
    "T00000LRiA3GTebvYkfweGrTuQtJ6Ktqs7qayZXT", "T00000LcmAhKuptNrcyPkwjs9Vih6wzapJe9Ssxr", "T00000LhsEM6m8CFZuHDtMpaNiUkxzfuFd32Pf6M", "T00000LL5uXYeb7Vz3oKkqeeJhh7iZjmKhWJagU4",
    "T00000Leb9rCjPfjzbXbGsU9khbBEoy13r6DSeUc", "T00000LTv1p2sRC7PnLcPgFaDpyctQ8b2hyEAcnz", "T00000LSMcqiVotVtNzEz6CQbozCMbmcMVSDjiTt", "T00000LLnBEMm3whBk1hBwGxVaC2iUSBuzy495aL",
    "T00000LaPaUFP7tsoqtNJocLQe9LypE4z152Didv", "T00000LdFj1cZ1Wh284TMN4Yr6uCVKHTBrayXZFr", "T00000LMAbjWYaDjuCPYsUuGjRgRA6uKUjPLW57D", "T00000LP4qwBsrrKKV4C8SiUMPdiaNKfX8RkH5LY",
    "T00000LfratX4TsWYKFQAfe6gPtcQLk6FKNkrxqR", "T00000LgbadWt6KK5daHcjjiE8vh9kM24R8W3y2q", "T00000LS17VaZ9G7XuVczwD8zS7gQfRNmCh2yXZs", "T00000LZJqD6jJ2nhtEzpX89w1hszS79cZZRund5",
    "T00000LKyu2mtcFonEAxpfyRQGHpKYSzVHy2GgL1", "T00000LexmUJaYJs2fuFeKC2sQBJxbJW5gZrj5va", "T00000LYQUdoVtCb1DNPtwKFbDESQ45wAh4mkitX", "T00000LTmVYXqvzUTyqg2WfrgpmPmAGzL7rePCyZ",
    "T00000LaCoE9rZL9TWtbD8P3u8SakoAScydBevLt", "T00000Lfi8zD5VwBhAzuDop35V55MCJVnkzFSZj7", "T00000LY1MFrYzau6MboBw7mJpTiDFX7TLovDQok", "T00000LfSaKQwvprEXUbCfi3Xwcbh4UXAGLnAqUx",
    "T00000LeqUA6ZR2YR1ZWP2489otdt2vFCPjsM74L", "T00000LNDUaDGEBp1xDy3tRPRWSrvnT67idy1tzH", "T00000LZJrHTjtapBbWmPjE5kaYrytAkstjq2yRN", "T00000LeqRwUrqK98X5WYdCaj9Luymuq7rrsuAYz",
    "T00000LdLfi9AVpSn1mNnqDXRJueiuK1RTFqtcg5", "T00000LZMxojqEVZtZMB53tyL5dL9oAWxYZDaeyR", "T00000Ld64vZVmCCLt67RG3NoSFVu5quLrd4Jzi2", "T00000LbLcsJHU7ZidMiU1fysvNRWyna14iDFxKq",
    "T00000LR27pwMMUdkNgPyjnQTQki2KLpwkp18Yds", "T00000LeatMrz1gnP71QY7ePM1TjuUULGQbmePVF", "T00000LMmiUprN6HffHP1HeMu1CdV9tWd4BzJaAm", "T00000LYYAfmoioBnXazR7qrVBWq2bNkSZdnAJJb",
    "T00000LcWN4JGY54h8h2NhJupTjdm5NJxroqF4YS", "T00000Lc7sM3XXyRvJcRKhhMn7zhzDUW4BBcmLFi", "T00000LYDoKVZ1vcTRAHX9zyAMRboiaY46NtnKML", "T00000LKKnC3cKopBXn4gbaqTVCfTR2HAdjoB6Kn",
    "T00000LYdyE2xyvwGCaqu3S7U6iLUAEKTNNuikVG", "T00000LTbu52SN3S1bTw459HX4iALTWdkw7FvjZB", "T00000LQgoULGZp6YAGwP76jMH3StPjmhvLykxWV", "T00000LUQrpFaRAv3M7euHX6rfFApJrvRDMykENU",
    "T00000LSZkQcFGYbjcc5p94kSPpzP62KrigergFz", "T00000LQJNW9GXTNBvWsQDiyUbXfjVPYAGNtH81Y", "T00000LPbW7Pn6hF3T8ujK5MgJcXZs7wLfAXjLad", "T00000LQ17hzzNAP9FkxmAupDYzqz4BktENyHGt1",
    "T00000LVMkBSh6hxHZE35G7fpdWDF8u5yeRuHkKb", "T00000LNqt4j8ve2yPJTJ8GGqTxfuZFuZXcuBegj", "T00000LZTr5Xf76wvmnD9HNeJDpBQRd8cR2udex8", "T00000LLRNGo1PtQsXs5wfmMaTzrkbZXKmMu4RgJ",
    "T00000LZKaWSh7QSrMCQ6gb2crN2LyijuD4NPvMx", "T00000LNY2XEADoaEUGQyKBkC9qanwHbjkihN4fe", "T00000Lefj9Nf8Z6fzavorGJhqt4R3izDoLVeVRJ", "T00000LTsAUTsB6mBSEzg53qXopa5caQPnydNEiH",
    "T00000LM4JHV4gEHyo6KtNRiqQeik36AqRmW3t24", "T00000LbgJouVrboeyPoUg3duVxsndg3ETrWNtSX", "T00000Li44D3UyoV6f2VhsUSZQ7hWRyTpPFm2ouG", "T00000LNvFy6TyPfsKAmjaDBMKUgYBmdrnSDdg8h",
    "T00000LTwUZ1tBz2obxgXT3CoFCxU5yiFu9LmARD", "T00000LWiAzHyPCEsxr9U7GyFew3pud1Pc2A9VMr", "T00000LbarLwR3GFvcn5M8PbKLoMgZp1yJzkGhdP", "T00000LLJReFnvbM7aKyoh3vwdH3RdkyAHXJWurw",
    "T00000LaKhXPKwuTYVEG2Df4i9WyNLEBjw7VQs6p", "T00000LeVgzWgRr4ZHsKEpWuWHyH24mzsvcJpR1j", "T00000LgnCuJ1gvpgL3tc9AUMZNFKC3dQMmRH9rH", "T00000LRr6eSPgFJwwVssjVkCKwsRKf8YhRrU82C",
    "T00000LXhz9HJcJjhstbT9GkGkLrA1tMKhzmbAnB", "T00000LKNWRAhUzCafqeUXXgsxcZ4qzXcoK6ZsM5", "T00000Le9peGQfNLiuHuEfENzfMWdDxSFwpWdnEf", "T00000LW57R1JZx7NHaLw3An9aDN2DS9AeydhiPt",
    "T00000LSmtLBcvHH2qTm7soyE7DvyCs7A2XU5XbK", "T00000LddCFPHKUMWWNGjZxC9Y4RUzMuAWfrJc2k", "T00000LXqHmnaCtu5bkeYseyvvv6wG8oN6SMFDQU", "T00000LMEregHSiLYSVbu95hCEnvEfSsf5faZmHz",
    "T00000Le6rve5ZT7cvjRJwKK5KCkUTzJQtW4BT1f", "T00000LajmTkWE9JZiFfFXoLax647HuUNV4pfD4o", "T00000LL1dSgyfuhTWBk4PqpTnnwuYSUZQ96ti1j", "T00000LXc7U9KDwHJtWXaQa7X1W3iXkR4LWvkrFn",
    "T00000LXj8c7G3NsDFxTwXo2pVg1KBFXyqmChn4r", "T00000LPzAXCFyKdsRHq4aGFC8CXQooZ7Quk9LxN", "T00000LPvrfkRYqW8Y2GxQCGw5VoHLUs4CU8tYeJ", "T00000LRdBFpyaUQePwSLXunPB5ZchS58qagvEQQ",
    "T00000LLeXsYYdABm4ZWU3ECVF7qryQ3aMxCfPCF", "T00000LWCrmgavt1fgPqkUMYdU2PH47vi2JRjZDq", "T00000LSLaVTHXcqbeJuZXDn5gnby8AKPHvchGr4", "T00000LP16U3GsGDjsnEkT5TFrLe4kLn9uQeYix8",
    "T00000LNGzRb86B1uvitvsRwUJyXd3YzpE4oLYqn", "T00000LL8q9Y3WfjHAg33CDKHZ4gcbwiCFdoKhm5", "T00000Lg89y1b27auWXTL5VX1zzbQrcwEtp66bCL", "T00000LTHJBruUDHM3sXsSxdLQkdvwr9L5284yEt",
    "T00000LYAV3KdnbDyqUsj34t1qcvz1e63vpu9XGC", "T00000LS4ZVhRjH6Z8HjsnZNXL7cwAPhDMgHRo22", "T00000LRnsxaJisnyJSQRSwxAvhq7cyH4tZV1gLd", "T00000LhJx8FiWc2Vyek5NmoNn8sJNrarMCqXLyn",
    "T00000LT1SZnsFSE4L3zrghxVbFErMAX7wcgP927", "T00000LTcF2PsxqdDqJoZ3xnwFmF5GLKTY3J6HgY", "T00000La77EyotMEsP3nHVVbnWrMeBsCvmogCMpj", "T00000LVTtf7D11YDjz8nBsRkWzbLsk8SWfYn1Xn",
    "T00000Lg8BhEbuDrzSrJMTqQAa1ypCAdB5hPrWSi", "T00000LRVQYa3xkZaXDebRz1GYGCJR2aKRaeMg7T", "T00000LKJK3vS7NNmKct6MxKhqwQP3e9bNtg1VVM", "T00000LLAauhiuijnpNzpguDVU1ZZdGLo157hvK7",
    "T00000LSGvkZgecnQbeePUTLVWGmgvfJQfZi7XdH", "T00000LMiLNRdM89pjHEsNzAB2KhViir2n8ANmwN", "T00000Lcbdf1z36V87Vz2M8zUaqPVjr2Kfnv9mvy", "T00000LYDroPHKkhemsgPVUfKqPd3sLkLEumYaip",
    "T00000LRwfioThdMQrhzHqA3grsymwz7yStqRHQ5", "T00000LVeAhsUdh2T1RTNcvXfZCZVjykuyVqGQZp", "T00000Ldb6X75659EPnZducFyrgV4rGbN595KjuX", "T00000LWvSSCeysARzqQ91g3tFTmDzZeSynSdJUA",
    "T00000LNzZMPo3u52F3EEiZZ7vFWiAeVmN3yErSD", "T00000LLbSqxBLtkusWUxPLKQQYqzxaghoZW2qoH", "T00000LiNUvhQ55wjqSc2YFppNzSNjmM1vvAd6P6", "T00000LW3vbZoeg6uzXo3tbnwAW9BwecCrh72EU3",
    "T00000LNfbttzCPUF2RiUaLjB7bPKob4P8ZnTTF1", "T00000LLDEbSkXC4YCxcxRzKbm5wTYxXxyELtfgG", "T00000LZnqCwGGCaSTQX76gQccdiRQDo5hd1BucD", "T00000La8Ttrrv2tDZKgkGHdeDHbbGUKgLuvKYbw",
    "T00000LiJaRvix1k7zEFJwjdc2hsE2QFjQ8tPMav", "T00000LPjCSvN3gngJDbu4DnsUPefrAWoybi4PPf", "T00000LPhTHgHWm573DWJ8uNBy7tgHcFw67Synmz", "T00000LXwFPd7cmrEid5NMxysDpPxC6iqVYWNxxS",
    "T00000LgTqC9DGq2J6ph5hGM8LcVY1D6X4UhPQoF", "T00000LUNNP8Fg5FUmCcMhr7kfrWM63a1PzdLzrB", "T00000LhPEa25QoFBPpcha3hTPA1MJDKEaSWAZs4", "T00000LPdwNn9VaYU83rWPwdBogwNkXpWtp5oqUQ",
    "T00000LQayjAiq6fZFz7XRdFmVz1L72WgAyyLrA5", "T00000LQzpmZ4kiRg9AajGh8F2LwHAUas3DiUVET", "T00000LgpuPZxsYBWQKML3vfmoZMh8DadJSmSyuJ", "T00000LV7hTgxgSyuU4UdnHfUZf4YPBkMUaoghDc",
    "T00000LRHaLCHewiWpMeCEpTrwYUz3UqbXbaSLrd", "T00000Lew75b29MeoujDBbyF1J4K65qiiGhY8atJ", "T00000LMBqu6nRYiawo3b14MfxAqiX7G15StRXkU", "T00000LbBoKu1HgKgt9YGZjPdLmURi6sX5e4ru3E",
    "T00000LeE1bUx9H4am2KWwfXnk3YHiSwVzCHYH4J", "T00000LLa97ahY53LaZk5Nm8hDEaHdurua5gvux7", "T00000LPy6syvNUeZectLCnET2pQ5LxYZEgct8kF", "T00000Li4cx8v7VWTzM2XHTPKUaNZ9L3S21RuZZy",
    "T00000LVdrEvrjU1RvHHK29KieerAYuxjS4V6SCt", "T00000LaKo3nyzNLvDNjFjyrW665VyiGtx5oZ9c6", "T00000LV4DAa49DDqtbeZA7sS8Zvxh6N6k7Ejpno", "T00000LTt4HLQdMKv2kV4wHVaguAMwQqLcS6pFWT",
    "T00000LPZQMcTrQNk4h4V8ocWX6fCKEg8MX47TeS", "T00000LZyvLZRR7RVdagJFzMwz2SpU1xRqbTYGcu", "T00000LZ7Tto5xhvh1SFfQP1NxBT27H4wTjyaTSg", "T00000LQmMEhtKCq5xXPgwuxEqB52ubje1yTiEfX",
    "T00000LSWHuQM8sM48ThbsG8P4Xu9zFZYqiHDQUM", "T00000LhGa3oemqbwRbyx8QrexhHBYqYrLZjhXzd", "T00000LN7vub6CwRQTNaM4psEreqNoLr4V1GHV74", "T00000La8iDpvaAKjQdTZmbDvGUQmTWq1Z4j2qjU",
    "T00000LYf5uu5WEG92XdNa7bWHDKy8hmedeybiqi", "T00000LQjKsKs8SAvDi8hZKC13tvfgZSq9pAekZD", "T00000LhPqChR1WSKpGLfCrFe243wkk6jph6coGG", "T00000LUXazNHvfuwTn9Cfrsc7KewCBBymQNMFuw",
    "T00000LXdKpAZdvZm7uXgFdWgaBM6JFD2CUcMhyn", "T00000LbASLAbkcQNmrkt8exbEyRnDBcYDDjMs9m", "T00000Lav7o92pNkKe3kcsneYth2uMJn8BdkeT48", "T00000LUctgNQqqf2wMggDu9aKdVjkX4J6YMqF12",
    "T00000LUH695VeXpqjRH6zGQbJud3NyvmVSudtnJ", "T00000LL94wt5vbg6XjWayv2WHFRNpvxLUa4x7tT", "T00000Lgx4p4KfqPEqkP1BLFKw6xp4j6DKQ9Rq11", "T00000LQALQWt5vq1MTJeyjDydf8TiEbVweRrt4u",
    "T00000LLUsmingYs1GD55swwrHVgiwih2jmGNZEA", "T00000LNFYEapqMnPQMA3SSdoEF147sHX2ygyUMQ", "T00000LStaptkRnGe737a5mKGwrYhTrztBieWE3z", "T00000Lat5fv5oh82oZ27YVPyxv5CaBBetqiAdUD",
    "T00000LhDuyRsUBG3K447a3EJK37jszEU4MToZfg", "T00000LZXsZMMo2Ysi98rEBHjLyubPQKy6KUBHpY",
};

using contract_t = top::xvm::system_contracts::reward::xtable_reward_claiming_contract_t;

class xtop_test_reward_claiming : public testing::Test {
public:
    xtop_test_reward_claiming() {
        for (size_t i = 0; i < test_accounts.size(); i++) {
            m_test_accounts.insert(std::make_pair(test_accounts[i], i));
        }
    }
    ~xtop_test_reward_claiming() {
    }
    std::map<string, uint32_t> m_test_accounts;
    uint32_t m_test_accounts_num{test_accounts.size()};
    contract_t m_contract{common::xbeacon_network_id};

    std::map<string, string> voters;
    std::map<string, std::map<string, string>> pledge_votes_map;
    std::map<string, uint64_t> stored_expire_token_map;
    std::map<string, std::map<string, uint64_t>> votes_table_map;
    std::map<string, string> adv_votes;
    common::xlogic_time_t m_timer_height;

    void set_voters(string voter_account, string adv_account, uint64_t voter_num) {
        auto iter = voters.find(voter_account);
        std::map<std::string, uint64_t> votes_table;
        if (iter != voters.end()) {
            auto const & vote_table_str = iter->second;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_table_str.c_str(), (uint32_t)vote_table_str.size());
            stream >> votes_table;
        }
        votes_table[adv_account] = voter_num;
        base::xstream_t stream(base::xcontext_t::instance());
        stream << votes_table;
        string v1 = string((char *)stream.data(), stream.size());
        voters[voter_account] = v1;
    }

    void set_pledge_votes_map(string account, uint64_t lock_time, uint16_t duration, uint64_t vote_num) {
        auto m = pledge_votes_map.find(account);
        std::map<string, string> pledge_votes;
        if (m != pledge_votes_map.end()) {
            pledge_votes = m->second;
        }

        base::xstream_t first_stream{base::xcontext_t::instance()};
        first_stream << duration;
        first_stream << lock_time;

        base::xstream_t second_stream{base::xcontext_t::instance()};
        second_stream << vote_num;

        string first = string((char *)first_stream.data(), first_stream.size());
        string second = string((char *)second_stream.data(), second_stream.size());

        pledge_votes[first] = second;
        pledge_votes_map[account] = pledge_votes;
    }

    void set_stored_expire_token_map(string account, uint64_t expire_token) {
        stored_expire_token_map[account] = expire_token;
    }

    void set_now_time_height(uint64_t height) {
        m_timer_height = height;
    }

    void call_test_function() {
        m_contract.calc_section_votes_table_and_adv_vote(voters, pledge_votes_map, stored_expire_token_map, votes_table_map, adv_votes, 1, m_timer_height);
        std::cout << "call over" << std::endl;
    }

    void print_result() {
        std::cout << ">>> print adv_votes size: " << adv_votes.size() << std::endl;
        for (auto const & v : adv_votes) {
            std::cout << "> adv: " << v.first << ", gain_valid_sum: " << base::xstring_utl::touint64(v.second) << std::endl;
        }

        std::cout << ">>> print votes_table_map size:" << votes_table_map.size() << std::endl;
        for (auto const & v : votes_table_map) {
            for (auto const & vv : v.second) {
                std::cout << "> account: " << v.first << " {adv: " << vv.first << ", gain_valid_num: " << vv.second << "}" << std::endl;
            }
        }
    }

    void assert_adv_votes(string adv, uint64_t num) {
        string adv_gain_sum = adv_votes[adv];
        EXPECT_EQ(num, base::xstring_utl::touint64(adv_gain_sum));
    }
    void assert_votes_table_map(string account, string adv, uint64_t num) {
        std::map<string, uint64_t> votes_table = votes_table_map[account];
        uint64_t account_to_adv_num = votes_table[adv];
        EXPECT_EQ(num, account_to_adv_num);
    }
    void assert_sum() {
        std::map<string, uint64_t> new_adv_votes;
        for (auto const & iter : votes_table_map) {
            std::map<string, uint64_t> votes_table = iter.second;
            for (auto const & iter2 : votes_table) {
                string adv = iter2.first;
                auto v = new_adv_votes.find(adv);
                if (v != new_adv_votes.end()) {
                    new_adv_votes[adv] = v->second + iter2.second;
                } else {
                    new_adv_votes.insert({adv, iter2.second});
                }
            }
        }
        EXPECT_NE(0, new_adv_votes.size());
        EXPECT_EQ(adv_votes.size(), new_adv_votes.size());
        for (auto const & iter : adv_votes) {
            auto v = new_adv_votes[iter.first];
            EXPECT_EQ(v, base::xstring_utl::touint64(iter.second));
        }
    }
};

using xtest_reward_claiming_t = xtop_test_reward_claiming;

TEST_F(xtest_reward_claiming_t, test_demo) {
    // current timer height
    string account = "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb";
    string adv = "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD";
    set_now_time_height(259200 + 10);
    // voters: {T80000b2a8697e7a72542637d1b6c696642ad97bec6feb: {T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD: 1000}}
    set_voters(account, adv, 1000);
    // pledge_votes_map: {T80000b2a8697e7a72542637d1b6c696642ad97bec6feb: {11248201_30: 5000}}
    // expired:  m_timer_height - lock_time >= duration * 24 * 60 * 6
    // m_timer_height - 10 >= 30 * 24 * 60 * 6 = 259200
    set_pledge_votes_map(account, 10, 30, 5000);
    set_pledge_votes_map(account, 20, 30, 2000);
    set_pledge_votes_map(account, 0, 0, 600);
    // stored_expire_token_map: {T80000b2a8697e7a72542637d1b6c696642ad97bec6feb: 300000000}
    set_stored_expire_token_map(account, 300000000);

    call_test_function();

    // calculation details
    string adv_gain_sum = adv_votes[adv];
    std::map<string, uint64_t> votes_table = votes_table_map[account];
    uint64_t account_to_adv_num = votes_table[adv];
    // stored_expire_token = stored_expire_token_map[account]; 300000000
    // calc_expire_token = if (expired && duration !=0 ) {return get_top_by_vote(vote_num, duration)}; get_top_by_vote(5000, 30) => 5000000000
    // unexpire_vote_num = if (!expired) ; 2000
    // valid_vote_sum = (stored_expire_token + calc_expire_token) / 1000000 + unexpire_vote_num; (300000000 + 5000000000)/1000000 + 2000 = 7300;
    // vote_sum = add(pledge_votes_map.second); 5000 + 2000 + 600 = 7600
    EXPECT_EQ(960, account_to_adv_num);                         // 1000 * 7300/7600 = 960
    EXPECT_EQ(960, base::xstring_utl::touint64(adv_gain_sum));  // adv_gain_sum + 960 = 960

    assert_adv_votes(adv, 960);
    assert_votes_table_map(account, adv, 960);
    assert_sum();
    // print_result();
}

TEST_F(xtest_reward_claiming_t, test_one_account_vote_one_adv) {
    set_now_time_height(4924800 + 10);
    string account = "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb";
    string adv = "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD";
    set_voters(account, adv, 1000);
    set_pledge_votes_map(account, 10, 570, 5000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 , expired, get_top_by_vote(5000, 570) -> 2500000000 token
    call_test_function();

    // valid_vote_sum = 2500000000/1000000 = 2500
    // vote_sum = 5000;
    assert_adv_votes(adv, 500);                 // 1000 * 2500 / 5000 = 500
    assert_votes_table_map(account, adv, 500);  // 500 = 500
    // print_result();
}

TEST_F(xtest_reward_claiming_t, test_one_account_vote_more_adv) {
    set_now_time_height(4924800 + 10);
    string account = "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb";
    string adv1 = "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD";
    string adv2 = "T00000LNmcQCfjjk1Dj691L75egthyzyBAdkRtx4";
    set_voters(account, adv1, 1000);
    set_voters(account, adv2, 500);
    set_pledge_votes_map(account, 10, 570, 5000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 , expired, get_top_by_vote(5000, 570) -> 2500000000 token
    set_pledge_votes_map(account, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;
    call_test_function();

    // valid_vote_sum = 2500000000/1000000 + 3000 = 5500
    // vote_sum = 8000;
    assert_adv_votes(adv1, 687);                 // 1000 * 5500 / 8000 = 687
    assert_adv_votes(adv2, 343);                 // 500 * 5500 / 8000 = 343
    assert_votes_table_map(account, adv1, 687);  //  687
    assert_votes_table_map(account, adv2, 343);  //  343
    assert_sum();
    // print_result();
}

TEST_F(xtest_reward_claiming_t, test_more_account_vote_one_adv) {
    set_now_time_height(4924800 + 10);
    string account1 = "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb";
    string account2 = "T80000a36a6d82fa55894ea36d7b043522b808fef45695";
    string adv1 = "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD";

    set_voters(account1, adv1, 1000);
    set_pledge_votes_map(account1, 10, 570, 5000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 , expired, get_top_by_vote(5000, 570) -> 2500000000 token
    set_pledge_votes_map(account1, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;    set_pledge_votes_map(account, 10, 570, 5000);  //

    set_voters(account2, adv1, 500);
    set_pledge_votes_map(account2, 10, 570, 3000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 ,expired, get_top_by_vote(3000, 570) -> 1500000000 token
    set_pledge_votes_map(account2, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;

    call_test_function();
    // account 1:
    // valid_vote_sum = 2500000000/1000000 + 3000 = 5500
    // vote_sum = 8000;

    // account 2:
    // valid_vote_sum = 1500000000/1000000 + 3000 = 4500
    // vote_sum = 6000;
    assert_votes_table_map(account1, adv1, 687);  // 1000 * 5500 / 8000 = 687
    assert_votes_table_map(account2, adv1, 375);  // 500 * 4500 / 6000 = 375
    assert_adv_votes(adv1, 1062);                 // 687 + 375 = 1062
    assert_sum();
    // print_result();
}

TEST_F(xtest_reward_claiming_t, test_more_account_vote_more_adv) {
    set_now_time_height(4924800 + 10);
    string account1 = "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb";
    string account2 = "T80000a36a6d82fa55894ea36d7b043522b808fef45695";
    string adv1 = "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD";
    string adv2 = "T00000LNmcQCfjjk1Dj691L75egthyzyBAdkRtx4";

    set_voters(account1, adv1, 1000);
    set_voters(account1, adv2, 2000);
    set_pledge_votes_map(account1, 10, 570, 5000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 , expired, get_top_by_vote(5000, 570) -> 2500000000 token
    set_pledge_votes_map(account1, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;    set_pledge_votes_map(account, 10, 570, 5000);  //

    set_voters(account2, adv1, 500);
    set_voters(account2, adv2, 1000);
    set_pledge_votes_map(account2, 10, 570, 3000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 ,expired, get_top_by_vote(3000, 570) -> 1500000000 token
    set_pledge_votes_map(account2, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;

    call_test_function();
    // account 1:
    // valid_vote_sum = 2500000000/1000000 + 3000 = 5500
    // vote_sum = 8000;

    // account 2:
    // valid_vote_sum = 1500000000/1000000 + 3000 = 4500
    // vote_sum = 6000;
    assert_votes_table_map(account1, adv1, 687);   // 1000 * 5500 / 8000 = 687
    assert_votes_table_map(account1, adv2, 1375);  // 2000 * 5500 / 8000 = 1375

    assert_votes_table_map(account2, adv1, 375);  // 500 * 4500 / 6000 = 375
    assert_votes_table_map(account2, adv2, 750);  // 1000 * 4500 / 6000 = 750
    assert_adv_votes(adv1, 1062);                 // 687 + 375 = 1062
    assert_adv_votes(adv2, 2125);                 // 1375 + 750 = 2125
    assert_sum();
    // print_result();
}

TEST_F(xtest_reward_claiming_t, test_more_account_vote_more_adv_expired) {
    set_now_time_height(4924800 + 10);
    string account1 = "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb";
    string account2 = "T80000a36a6d82fa55894ea36d7b043522b808fef45695";
    string adv1 = "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD";
    string adv2 = "T00000LNmcQCfjjk1Dj691L75egthyzyBAdkRtx4";

    set_voters(account1, adv1, 1000);
    set_voters(account1, adv2, 2000);
    set_pledge_votes_map(account1, 10, 570, 5000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 , expired, get_top_by_vote(5000, 570) -> 2500000000 token
    set_pledge_votes_map(account1, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;    set_pledge_votes_map(account, 10, 570, 5000);  //

    set_stored_expire_token_map(account1, 2000000000);
    set_pledge_votes_map(account1, 0, 0, 3000);

    set_voters(account2, adv1, 500);
    set_voters(account2, adv2, 1000);
    set_pledge_votes_map(account2, 10, 570, 3000);  // 4924800 + 10 - 10 >= 570 * 24 * 60 * 6 ,expired, get_top_by_vote(3000, 570) -> 1500000000 token
    set_pledge_votes_map(account2, 20, 570, 3000);  // 4924800 + 10 - 20 < 570 * 24 * 60 * 6 , not expired, 3000 ticket;

    set_stored_expire_token_map(account2, 1800000000);
    set_pledge_votes_map(account2, 0, 0, 2000);

    call_test_function();
    // account 1:
    // valid_vote_sum = (2500000000+2000000000)/1000000 + 3000 = 7500
    // vote_sum = 5000+3000+3000 = 11000

    // account 2:
    // valid_vote_sum = (1500000000+1800000000)/1000000 + 3000 = 6300
    // vote_sum = 3000 + 3000 + 2000 = 8000
    assert_votes_table_map(account1, adv1, 681);   // 1000 * 7500 / 11000 = 681
    assert_votes_table_map(account1, adv2, 1363);  // 2000 * 7500 / 11000 = 1363

    assert_votes_table_map(account2, adv1, 393);  // 500 * 6300 / 8000 = 393
    assert_votes_table_map(account2, adv2, 787);  // 1000 * 6300 / 8000 = 787
    assert_adv_votes(adv1, 1074);                 // 681 + 393 = 1074
    assert_adv_votes(adv2, 2150);                 // 1363 + 787 = 2150
    assert_sum();
    // print_result();
}
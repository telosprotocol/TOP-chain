#include "xtopcl/include/api_method.h"

#include <gtest/gtest.h>

#if 0

#    define init()                                                                                                                                                                 \
        xChainSDK::ApiMethod api;                                                                                                                                                  \
        std::ostringstream outstr;                                                                                                                                                 \
        api.set_keystore_path("/home/charles/topnetwork");

#    define reset_output() outstr.str(""), outstr.clear();

#    define PSWD_PATH "./pswd"

// #    define ENABLE_MANUAL

/*
Subcommands:
  createAccount               Create an account.
  createKey                   Create a worker key for specific account.
  listAccounts                List all accounts in wallet.
  setDefaultAccount           Set a default account for sending transactions.
  resetKeystorePwd            Reset the password for a keystore file.
  importAccount               Import private key into wallet.
  exportAccount               Export private key and keystore json file.
*/
TEST(test_wallet, wallet_createAccount) {
    init();

#    ifdef ENABLE_MANUAL
    api.create_account("", outstr);
    std::cout << outstr.str() << std::endl;
#    endif

    reset_output();
    api.create_account(PSWD_PATH, outstr);
    std::cout << outstr.str() << std::endl;
    EXPECT_TRUE(outstr.str().find("Successfully create an account locally!") != std::string::npos);
}

TEST(test_wallet, wallet_createKey) {
    init();
    api.create_account(PSWD_PATH, outstr);
    auto new_account = outstr.str().substr(outstr.str().find("Account Address:") + 17, 46);
    std::cout << "new_account: " << new_account << std::endl;
    // std::cout << outstr.str() << std::endl;

#    ifdef ENABLE_MANUAL
    reset_output();
    api.create_key(new_account, "", outstr);
    std::cout << outstr.str() << std::endl;
#    endif

    reset_output();
    api.create_key(new_account, PSWD_PATH, outstr);
    std::cout << outstr.str() << std::endl;
    EXPECT_TRUE(outstr.str().find("Successfully create an worker keystore file!") != std::string::npos);
}

TEST(test_wallet, wallet_listAccounts) {
    init();
    api.list_accounts(outstr);
    std::cout << outstr.str() << std::endl;
}

TEST(test_wallet, wallet_setDefaultAccount) {
    init();
    api.create_account(PSWD_PATH, outstr);
    auto new_account = outstr.str().substr(outstr.str().find("Account Address:") + 17, 46);
    std::cout << "new_account: " << new_account << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    reset_output();
    api.set_default_account(new_account, PSWD_PATH, outstr);
    std::cout << outstr.str() << std::endl;
    EXPECT_TRUE(outstr.str().find("Set default account successfully.") != std::string::npos);

#    ifdef ENABLE_MANUAL
    reset_output();
    api.set_default_account(new_account, "", outstr);
    std::cout << outstr.str() << std::endl;
#    endif
}

TEST(test_wallet, wallet_resetKeystorePwd) {
    init();
    api.create_account(PSWD_PATH, outstr);
    auto new_account = outstr.str().substr(outstr.str().find("Account Address:") + 17, 46);
    std::cout << "new_account: " << new_account << std::endl;
    auto new_account_pub_key = outstr.str().substr(outstr.str().find("Owner public-Key:") + 18, 88);
    std::cout << "new_account_pub_key: " << new_account_pub_key << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    reset_output();
    std::string wrong_pub_key_path = std::string("xxx");
    api.reset_keystore_password(wrong_pub_key_path, outstr);
    std::cout << outstr.str() << std::endl;
    EXPECT_TRUE(outstr.str().find("No file with public_key xxx") != std::string::npos);

#    ifdef ENABLE_MANUAL
    reset_output();
    std::cout << "--------- TEST HELP ----------" << std::endl;    // NOLINT
    std::cout << "-         old is 123         -" << std::endl;    // NOLINT
    std::cout << "-         set to \" \"         -" << std::endl;  // NOLINT
    std::cout << "------------------------------" << std::endl;    // NOLINT
    api.reset_keystore_password(new_account_pub_key, outstr);
    EXPECT_TRUE(outstr.str().find("Reset password successfully!") != std::string::npos);

    reset_output();
    std::cout << "--------- TEST HELP ----------" << std::endl;    // NOLINT
    std::cout << "-        old is \" \"          -" << std::endl;  // NOLINT
    std::cout << "- press ENTER(compatibility) -" << std::endl;    // NOLINT
    std::cout << "-    set to `{ENTER}` false  -" << std::endl;    // NOLINT
    std::cout << "-    set to `123`     ok     -" << std::endl;    // NOLINT
    std::cout << "------------------------------" << std::endl;    // NOLINT
    api.reset_keystore_password(new_account_pub_key, outstr);
    EXPECT_TRUE(outstr.str().find("Reset password successfully!") != std::string::npos);

#        if 0
    // not allowed empty now
    reset_output();
    std::cout << "--------- TEST HELP ----------" << std::endl;    // NOLINT
    std::cout << "-        old is \" \"          -" << std::endl;  // NOLINT
    std::cout << "- press ENTER(compatibility) -" << std::endl;    // NOLINT
    std::cout << "-    set to `ENTER`(empty)   -" << std::endl;    // NOLINT
    std::cout << "------------------------------" << std::endl;    // NOLINT
    api.reset_keystore_password(new_account_pub_key, outstr);
    EXPECT_TRUE(outstr.str().find("Reset password successfully!") != std::string::npos);

    reset_output();
    std::cout << "--------- TEST HELP ----------" << std::endl;  // NOLINT
    std::cout << "-        old is empty        -" << std::endl;  // NOLINT
    std::cout << "-        press ENTER         -" << std::endl;  // NOLINT
    std::cout << "-         set to 123         -" << std::endl;  // NOLINT
    std::cout << "------------------------------" << std::endl;  // NOLINT
    api.reset_keystore_password(new_account_pub_key, outstr);
    std::cout << outstr.str() << std::endl;
    EXPECT_TRUE(outstr.str().find("Reset password successfully!") != std::string::npos);
#        endif
#    endif
}

TEST(test_wallet, wallet_importAccount) {
    init();

#    ifdef ENABLE_MANUAL
    api.import_account(PSWD_PATH, outstr);
    std::cout << outstr.str() << std::endl;

    reset_output();
    api.import_account("", outstr);
    std::cout << outstr.str() << std::endl;
#    endif
}

TEST(test_wallet, wallet_exportAccount) {
    init();

#    ifdef ENABLE_MANUAL
    api.create_account(PSWD_PATH, outstr);
    auto new_account = outstr.str().substr(outstr.str().find("Account Address:") + 17, 46);
    std::cout << "new_account: " << new_account << std::endl;
    auto new_account_pub_key = outstr.str().substr(outstr.str().find("Owner public-Key:") + 18, 88);
    std::cout << "new_account_pub_key: " << new_account_pub_key << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    reset_output();
    api.export_account(new_account, outstr);
    std::cout << outstr.str() << std::endl;
#    endif
}

#endif
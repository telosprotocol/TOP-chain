#include <string>

static std::string const g_consortium_test_genesis_config =
    R"T(
{
    "genesis": {
        "accounts": {
        "tcc": {
            "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB": {
            "balance": "1000000000"
            },
            "T00000LNEZSwcYJk6w8zWbR78Nhw8gbT2X944CBy": {
            "balance": "1000000000"
            },
            "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo": {
            "balance": "1000000000"
            }
        },
        "genesis_funds_account": {
            "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73": {
            "balance": "2999997000000000"
            },
            "T00000LTHfpc9otZwKmNcXA24qiA9A6SMHKkxwkg": {
            "balance": "6000000000000000"
            },
            "T00000Ldf7KcME5YaNvtFsr6jCFwNU9i7NeZ1b5a": {
            "balance": "3400000000000000"
            }
        }
        },
        "seedNodes": {
        "T00000LeEMLtDCHkwrBrK8Gdqfik66Kjokewp23q": "BPZmAPKWLhhVDkJvWbSPAp3uoBqfTZG0j2QLyOaT5s3JqOxjIvTQFnmBXNUiMV3xwJ/bp9Sq7vD47fvAiGnC4DA=",
        "T00000LVeEo7QGURbT7Kxc5SxMmMxvbhXXc1r2aq": "BCXnLl94yQR8L1pLdz5SqEhHIkjifycTNYms44y80OrzSAa1zJ7d8umPcccTxiFaRbqPQvfWmdsr560809aX1bI=",
        "T00000LMhJMWwmFMwTfpfrCzqtQQ9DBveVUirkSC": "BMz9dh7/AozzPXzP8vq5jaMKtK7YYTM0YO/C7P4jFWi33kfZo9poTjAceSRJJ5lO8ch+nM71nUuLpqYxgZ+zRpE=",
        "T00000LScQir9BbHc2YW5iE1kVYp6ncYds337aCo": "BD1vW7NQad07ppFtPEuepXjklHzbSJNoTXYBFfM5hD8uRxiwcQJLp053MdVhoI77iB3/NfEK6q//VvP/MNN+yFk=",
        "T00000LV1ooVsoMHfL6MnCDx64Hdg4vchnbfaGSW": "BPDmX32DPqJRb+QzskUw2SpSoRxn8ObCIE3JYl0ZI6rOKDRsVXTrjDDOtWICHl0CVjV0N2Ez3pWiWFiNsgg8Nfw=",
        "T00000LSvT8hqFL39GvmuotQBvksALLcubR3ivRb": "BECRZwpagglNLIjzXvNWqyaTo3pCZS8wT9siQj+ZURPCHqOVd2e6O/G5aRHcm6y6wrNpjzpmDs/23zDYiaUu/KA=",
        "T00000LcFkZzzADMS6tJnzC6XH5N79ZCrq6UHrje": "BLKB1F4P0a9JDzVxhuoZfIjsPa1LgecC1QkoIdepjtBH5mfXVebncbOPoPte69q/yhmL5fBipnY9xg0o1cnemnI=",
        "T00000LTLK35EK52a4mzTCSKxW9BmHUqqNKGoDmX": "BFJiEebMdm8tjC2dB60G0OvnpbKkBhe6zBQ+F9i4dIRtyn6Wv5NVN4NpsDxN+04duNuSfZQ0JxUhPz8e3I3aZAI="
        },
        "timestamp": 1599555555
    },
    "chain_name": "new_horizons",
    "chain_id": "9999",
    "platform_public_endpoints":"192.168.50.135:9961,192.168.50.136:9961,192.168.50.137:9961,192.168.50.138:9961,192.168.50.139:9961",
    "platform_url_endpoints": "http://unreachable.org/",
    "root_hash":""
}
)T";



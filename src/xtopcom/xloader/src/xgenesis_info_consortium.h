#pragma once

#include <string>

static std::string const g_ci_consortium_genesis_config =
    R"T(
{
    "genesis":{
        "accounts":{
            "genesis_funds_account":{
                "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73":{
                    "balance":"2999997000000000"
                },
                "T00000LTHfpc9otZwKmNcXA24qiA9A6SMHKkxwkg":{
                    "balance":"6000000000000000"
                },
                "T00000Ldf7KcME5YaNvtFsr6jCFwNU9i7NeZ1b5a":{
                    "balance":"3400000000000000"
                }
            },
            "tcc":{
                "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB":{
                    "balance":"100000000000"
                },
                "T00000LNEZSwcYJk6w8zWbR78Nhw8gbT2X944CBy":{
                    "balance":"100000000000"
                },
                "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo":{
                    "balance":"100000000000"
                }
            }
        },
        "seedNodes":{
            "T00000LeEMLtDCHkwrBrK8Gdqfik66Kjokewp23q":"BPZmAPKWLhhVDkJvWbSPAp3uoBqfTZG0j2QLyOaT5s3JqOxjIvTQFnmBXNUiMV3xwJ/bp9Sq7vD47fvAiGnC4DA=",
            "T00000LVeEo7QGURbT7Kxc5SxMmMxvbhXXc1r2aq":"BCXnLl94yQR8L1pLdz5SqEhHIkjifycTNYms44y80OrzSAa1zJ7d8umPcccTxiFaRbqPQvfWmdsr560809aX1bI=",
            "T00000LMhJMWwmFMwTfpfrCzqtQQ9DBveVUirkSC":"BMz9dh7/AozzPXzP8vq5jaMKtK7YYTM0YO/C7P4jFWi33kfZo9poTjAceSRJJ5lO8ch+nM71nUuLpqYxgZ+zRpE=",
            "T00000LScQir9BbHc2YW5iE1kVYp6ncYds337aCo":"BD1vW7NQad07ppFtPEuepXjklHzbSJNoTXYBFfM5hD8uRxiwcQJLp053MdVhoI77iB3/NfEK6q//VvP/MNN+yFk=",
            "T00000LV1ooVsoMHfL6MnCDx64Hdg4vchnbfaGSW":"BPDmX32DPqJRb+QzskUw2SpSoRxn8ObCIE3JYl0ZI6rOKDRsVXTrjDDOtWICHl0CVjV0N2Ez3pWiWFiNsgg8Nfw=",
            "T00000LSvT8hqFL39GvmuotQBvksALLcubR3ivRb":"BECRZwpagglNLIjzXvNWqyaTo3pCZS8wT9siQj+ZURPCHqOVd2e6O/G5aRHcm6y6wrNpjzpmDs/23zDYiaUu/KA=",
            "T00000LcFkZzzADMS6tJnzC6XH5N79ZCrq6UHrje":"BLKB1F4P0a9JDzVxhuoZfIjsPa1LgecC1QkoIdepjtBH5mfXVebncbOPoPte69q/yhmL5fBipnY9xg0o1cnemnI=",
            "T00000LTLK35EK52a4mzTCSKxW9BmHUqqNKGoDmX":"BFJiEebMdm8tjC2dB60G0OvnpbKkBhe6zBQ+F9i4dIRtyn6Wv5NVN4NpsDxN+04duNuSfZQ0JxUhPz8e3I3aZAI="
        },
        "timestamp":1599555555,
        "root_ca":{
            "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB":"-----BEGIN CERTIFICATE-----\r\nMIIDbjCCAlYCCQD1pnLG7w+yejANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3MTEyMFoYDzMwMjIwNDA4MDcxMTIwWjB4\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJ\r\nKoZIhvcNAQkBFg1yb290QHRlc3QuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\r\nMIIBCgKCAQEA390SsOcffSZv4hvcFtKV15ruczuySIj8erY7VD0s3qyQmr49VMH7\r\nJIxFy7ZD74d17JffqOHc2p3raX7bFKA4X10Ezc+MaszfVgwavvmGSEJY6i88OQ1N\r\nXHhK7aLWSc23l40NKdZrKr08mgWlw3WuIUiNyvjGfXf0L82T81ApmWuMQCv3SK6k\r\nn1YX0kyS9BU6D6TBWiSF6ENFGsNEwj6KoNbpe9f8kvylI7jwRT3/ZtQVE182VnF7\r\nXh3FZWbA4PdD4A5TGrpp7BJnmpi6UR3xWnULUSVY6xki+HMkY/jwQp8eajNEr6wW\r\nDN5CZr0YaA2EyiZLOFQKWCINH+X2NRKVUwIDAQABMA0GCSqGSIb3DQEBCwUAA4IB\r\nAQCP+GTMrJpNYtxMZjTAw8yB+iAAhz62WQsiO4TVEourmhRWQluOhO5g0EjVYLHF\r\nonSSahELcU3I9/dcU2X4pMaOBvQDEQ/Z2t4jwAu665HNUeXbB8XV33qtOswvIVJQ\r\nrQ6CNKEhQkhGLz5QnZ8SQIfrP5TLVl8Oq+qfQiTGTnfZXDTa0+8e/+r/KorFB4uT\r\n9PN5bHUxMEocdcmJt0awNmes2NBf0rDuyLFvA9/8yGdsb3n04gSavP6ocx9gVf6n\r\nSg6m94U0ExdkYVlE5ZenZYhiGHXR46bSWnb8khaaw89aju1QbDcrODspJuzdylmW\r\nC+7rfURXWXa/tPCb/GeDFAuQ\r\n-----END CERTIFICATE-----"
        },
        "seedNodes_ca": {
            "T00000LeEMLtDCHkwrBrK8Gdqfik66Kjokewp23q":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+XzANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3MzM1NloYDzIwNzkwMTIxMDczMzU2WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNcDIzcUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAJ6huaHiObK1CFluGd376nwUVxJlXFCYYwrOpapo6zaRrLcc\r\nZY2ySolKXS48JdbzVScq5nNZk0Xv5q8jOIQUlyYHNu9g2EEuUV71fZnOhyjXWuGY\r\nfY7Qvp+r/iCZ9n8fXOJBiQGciuVMsu6we4JYqjRLN6gcQzwuCaiUIQs6eINq3bAo\r\nl5L2IunYCAkx9ZrDMWN3BO0KQOQ9aelNBy2pcd70G7yn1+icSV57ODNQFj4L9ow9\r\nujyiw4IdlGR5XiwlOumNoeKqLfAu6qyiGeJ45ZBpmS8XloS1716N7fDi+A/adeJz\r\nVyoimw9o/Q1y2rUiKML8cFYY4eO+PPLU+55sttcCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEARcdEzpnBRk1RE23dufAjm3fMAa+tCOgTqIIGv+OmyxK++T8UIQOAvLiL\r\nFQBaWmENzoPdbAG49dyovcHz9rxLcSt493SeWmB0+GGElyLNccOKaqikRHmcSG7Q\r\n4Fsijr3U0Ql+bgmMETs6pNKAv1z56rw3bULq48VMQlY388XVAADjXHoElvRxKhp2\r\nABCBKEJq3SgJGS5B9PVbvxtCPYH2Rl2djjBd5rQ5Ez6UZiGvIeyeOd+Moylf/l95\r\nK6ydQ79ZQadyzGrdZ6LG+DWIOIgTPqqFD+I8g6ja+Y6IRUMIm/EPAFjZbrZ5bXoo\r\nGhj1Po08WUxXPs+DUTrhpPQgP9uYTQ==\r\n-----END CERTIFICATE-----",
            "T00000LVeEo7QGURbT7Kxc5SxMmMxvbhXXc1r2aq":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YDANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3MzkwOFoYDzIwNzkwMTIxMDczOTA4WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNcjJhcUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAOOXwlsE0tzSkmX815v2v2TxbDFixe133iVH7Shn22BgsKNE\r\nX8OOXA+d6Cye6uSDiMIJa0LpZ+6wsBkgyJKHgnShAXglXJbAhT9/iKCzHEbErJz9\r\nq21b03QFPvxLxudkyZ0G4qLjqMLHHQoGpgH9wlTSKWNixojoFTo5H493H9dhqiYi\r\n2fG+AJaF1NLwX2vrRLYXJt53blO+oduZa/f/l/PP/EfPBreXUfsWHfvAv7zb8MbI\r\nRV1xAjyaoLsEfwLgi1bVMbDY1YNUgl8YYlIIP+nFrGoMc+BgXf9i4xSF8Ay5g4KO\r\nH/2ZoLof2PvjUuf2fxSZnV9HSc0875a4/mNgDjECAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAwyJ7So4cqZpR2k597R/sfBffXhbPSuKjiVpWj+cfnKk5ycLB+iU9HpyP\r\n8MlnVX0id9gl7vva8B2YBGBNA0gbww6Sn+ITpo9XUn5mLirPLmUwdqRNNgONzTTc\r\nvtiA5TDLrC6H8gbDcEMl8+eszxFjZ1OkVPiATG7UdIbyXi0WLJVAt0Uq23YHD6nC\r\nm1BgR4S26V+JEjcVAYG9XTn2fvJce6MUULuzwlatwqeXyp2dumGBE6bcrwnci0Xw\r\n75zHMJw9TNXT17S1u8VEfpeqX4U6a2AgmfdxP0KhS/IZSnYFY4GUhxWpVRA4ycoJ\r\nHHAwdeqx79Ru6K2jk5FUxDBQf9xkxQ==\r\n-----END CERTIFICATE-----",
            "T00000LMhJMWwmFMwTfpfrCzqtQQ9DBveVUirkSC":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YTANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NDMyN1oYDzIwNzkwMTIxMDc0MzI3WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNcmtTQ0B0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBALAIoWsnyEc3xIRTOSMbWjQq0mqSVjQCsbOPda1VmK/3HDFe\r\nr3y4E5Noy7jOLhbWeKHFdVIB0wDm7fDDes06yyZ/xiGTPOROYS6C/jolOlQkafLh\r\nBAem5vlKiPb8U3sZpvspoYIxExzyHqvLLN0j8rONnsZSl14X1WAarA+1gOg9+ZQc\r\nvQB1Bvu4yZxpaNOiZUPBbDsj5JfHFOM5NaN/jPHmUluXBydC2uzomYObSUB/2P+O\r\nE+eNMPoITVF3h4HsynxdmrqEcO1OZ4jACZTtr9MPKsrOAfQ17DZ7Gprx/T8d+4Gn\r\nE+cJ3L9Woo0+89SlUOrP8gjykddVNVOJuiCzet8CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAYDu5KHTgXiOF9FHOyLaoRDbddmBSAEAXISs8kEw0sV+Sihvj0Cj5FTaL\r\ncDgq8dXR0m4p3PY45yV652ZWP3Nn+yMvjXFQPICjeQo/Amr85zyflrM1TUo4HCex\r\nbYCFhCIsHgBHopvMsHR1xnKeqTNp5DeHlvH+U5j5qBdCiJG2LZbfOllF+7kg+bSn\r\n+1eIxFgVPEU0BIABOc5gDLhU0FpjAX+OOscmaEiSDs3XZ4yzAytPClwnVVSLmKLX\r\nqbIbYNUoZeSVYgp4+oFTI9x77H9QoK0YoeYRVbS5c3C21Ml94iTV+B2rkh1RXxT6\r\nWzYeGbIPIKhr0HHBMEnMNbGd/zrrKA==\r\n-----END CERTIFICATE-----",
            "T00000LScQir9BbHc2YW5iE1kVYp6ncYds337aCo":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NDY0OFoYDzIwNzkwMTIxMDc0NjQ4WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNN2FDb0B0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAOFs76bhWxDBoUEVioDbQCUvRrRuZzugj76i5Mwvm9azBZ3S\r\nJvqdp4BNCjLc2LKHs9k2bcVyz5YSnq4R0ITC2ozXBkvlusqiihfAUPDT5Yhvwyml\r\nHFbz/sCfs/PHm2NvbP78aKlz8ewxIolnAc4ReXd3zLruh6a9f4m9nt/XPNTbw4Z7\r\nBs8P3qHOE03OrfwlZUjrV784t1BJvD4BETS86dLXnFm2m5nw6e+lIgr84rvIyjOZ\r\nXKeUcvagnPuAcv0cs0yF8u899EHwrrGZX2ASq2/zXeL9GEMRAw3bMhYKpSl0lOsd\r\nn0YRp4btLxnvPGQ2SA9o93iim1bLhZu3M78xNt0CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAcC8BuAc1u3h0+R09ksVsSSM1R4g5vwl/8/tPRCn7LEk/RvM30WJGtnhy\r\nc2zVB82eYQH0ZUl3sYEvco3Sa/4RUerfHWuhgj+aZhLYBAaa9yBwaFzG694JjQKZ\r\nnIFst5CYfvhOs2S0WKnIqD4uikAiVEBbN5qqpLiWlM8b7wOROw3FnhTgEr+YQsOW\r\nYvcc+of3uc9T61nRoUit8zPMVmlsFYlyVuc7qWNU/gJ63MyrqL9iSZ0sG7OTxErX\r\nNzEGOxuiq1Wiqxg2HRBWnEU1gEd1RHlWtPeASSo58ErhU7GeqIl4YCH1qzLneIOB\r\nhjyVJ+dWls+yFhtCUBFkPdTRIwRq6w==\r\n-----END CERTIFICATE-----",
            "T00000LV1ooVsoMHfL6MnCDx64Hdg4vchnbfaGSW":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YzANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NTQxN1oYDzIwNzkwMTIxMDc1NDE3WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNYUdTV0B0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMXWcRwHeB/VvycxX5dzaE+XtiyPkaC42i7++RYhY4detUvz\r\ne4o+dpcomBzv1kL0HoxusXDCAZxmVYMJPycVg3x2RMSnHqONlyNr3JuE4Okc8gyq\r\n6IeackkVzKpc7qv3CyZ1v8RPuRSPF5qaphEGJ5N5fKmXaYQ4TJlqhWt0p1NEqAh3\r\nI2alidkEuAjffZxaLwmoEpv0Rp63IVvoQJ6ohFSbWRkwImmjtzKUnfbxIsT5Kp4o\r\nljcQXDuPwgx6gsmnCvruDmQpFMBxFiWqXbwP+k+NXj7623juGLVc63/FK26ZLOVE\r\nP6riClDMvYMBf1BwChOBx47bAigrNx5CBodmnhcCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAZMKC+9xNgVVd6XaAea7O+/ZP/Aw0CDNfmFIXp5allgIktzrHKs/z/xwW\r\nALsJZ4wBgknkKcBJ5Dwtu7I90NzkycKopdYyc5LaI0yo/c6/tV7gD64ZLXixbey2\r\ndjO8Tpp0UtTzRwY/a0MoAvUa/aoM9wXlh/K1r4fWl+QOjkObkaNx+foQEbJrMxIT\r\nTUe/CfJ0qyihE7sIE+u/H02+2THkdSWbE1ozaflRSHMIc/9fxzb1B/0a3NiLC66x\r\nOdqMdEJ3fa1eOXRJXwe0BIUZLirkpiYt7UxP+rHGGNu+lrkWYCvolGoYqRnWVXmO\r\ng0Btsxx9eegVjE9nT2SYTmqHARaw6Q==\r\n-----END CERTIFICATE-----",
            "T00000LSvT8hqFL39GvmuotQBvksALLcubR3ivRb":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZDANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NTg0NFoYDzIwNzkwMTIxMDc1ODQ0WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNaXZSYkB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAKoS8vN3pyUis0iQkfA/ZhRgJcGVh+SPohYg9slabsxab1/j\r\nQtubyhjUYEzayHkpOSg6odNhuEuv8gBQxnDAzwThvGMIvrAGnyOTFZjRmO+hpUf0\r\nZL7pV/0ksH1ctBnsgHcSeqR0oUm2eFE8VND7kz3kgF3zStvefj7OWbgQ6CkZT300\r\npvD9pfx/OdXc2J5XeIe6CD+gDgC1aa7sd/wP4QUavJ/iFvufintoBhZ649nQ4TPb\r\nIkPSCtsqhhAl8sQEJLuNIOXNeJCxOcEUr2bFDpCA1JovSPp01rU9iq1XN099Tb1z\r\nFaKmCiOsLqmUl33+BGxCISvylKisqx6d+1yorHUCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAkmT2W7mJ/x7xPY7+RApnAIJYJ5z0UQQx44AXdUDNXg7fgqOTiaNrf8PN\r\n8m9GdpFV4B83vyTf3GzIT64IG5CRupUDQTY/9wDlrQsHo0za4PZE2yQjYesNDQbo\r\n5NSBZ326nGXCIC6cKw/173OuXqptiaqJr9BZTFpG2m16vjtfg/BSkPj6BJp6dhKn\r\nXAf4R3Ty962z8dilhX0xA59XaV8aen32G348S4fLWOnw/oGr3+LdKSYXq6L8oFtv\r\nlhavyQEWu5hGe0+1driq5WkzKhGQG7H5i4bWWCEYLnjwzGcLLJgaD7iOZ8L80V6c\r\nhleJ2esQ3TxVa42Nixvvl9hjxyIJWg==\r\n-----END CERTIFICATE-----",
            "T00000LcFkZzzADMS6tJnzC6XH5N79ZCrq6UHrje":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZTANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDIwNloYDzIwNzkwMTIxMDgwMjA2WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNSHJqZUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMmVIhNcKMm73/HznUm6bGv6IU1sLqsSSh/cbwR+lM3oDpHv\r\nREGAMIQhwTZkznTJcpxMukFgwu4PQUcD3SPzyII4eE3/Nl1VBtQOFVNIUSLBUXfh\r\niNCwippeEzQf1LFiK9ELyIPghgNymvWViR3IErQLop8lIup3xFdmtLt3+E3X3vRM\r\n8z2v6mKUSpWMAQdWPhT3VwvJswrfRjoOZAXbeHSGoxx+YTiZnTBqMCtEI4+ddx2g\r\nCWaZEk0+D0ir509Ddzih3q4BSsZkJZvC2mx2Vjl4EFO32f6oXsAunkqamUD74gks\r\n9M3iu+IX3k8HOeWznjI/bUmW3TskDB0Vjb1G9FsCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAZvBzXzeRt98nOezS+6STh7ePx9g6++0onyqJyv1nKNQvfyzQQ3a3J45H\r\nblkeoy5qsqtWPlcPbB73juaCaNdkUp2SWUORnQ9CV7AQGsg/MM01/PNKG2eGplsa\r\nm2Q9uSxxXfhG36V64rWbmzOYByxzDtz2cVza/wYGf8vMFcMIMchrzvX4eFwr3aee\r\nX8HZw4DOyfemcGHjapj9v0xsXaXD2B5HZ4sRhdkRScb/pJZJweCoq1Ej8/+4TZ40\r\nZGER5ySlBwZ2e9TLp2TgG9MYisT1q5BZgekg5ePSKjEp79ibN3LX+Vihed7LsVH3\r\nKdfGkGgiBOuM1KM7TinV4y7TAMTIlA==\r\n-----END CERTIFICATE-----",
            "T00000LTLK35EK52a4mzTCSKxW9BmHUqqNKGoDmX":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----"
        }
    },
    "chain_name": "consortium"
}
)T";

static std::string const  g_dev_consortium_genesis_config =
    R"T(
{
    "genesis": {
        "accounts": {
            "tcc": {
                "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB": {
                    "balance": "100000000000"
                },
                "T00000LNEZSwcYJk6w8zWbR78Nhw8gbT2X944CBy": {
                    "balance": "100000000000"
                },
                "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo": {
                    "balance": "100000000000"
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
            "T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp": "BNRHeRGw4YZnTHeNGxYtuAsvSslTV7THMs3A9RJM+1Vg63gyQ4XmK2i8HW+f3IaM7KavcH7JMhTPFzKtWp7IXW4=",
            "T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446": "BN9IQux1NQ0ByBCYAAVds5Si538gazH3gNIS5sODadNRA2zvvKDTSKhfwX5GNWtvb0nmoGHjQp9J9ElMyOUwBkk=",
            "T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB": "BP+s96ilurhraFU7RD2Ua60rD8CpgDxCjWcp67yq7D500gf0ej5vBGiwqZ2GwoEWAcXFHqUlTQW8IqIWHCk5eKk=",
            "T00000LLJ8AsN4hREDtCpuKAxJFwqka9LwiAon3M": "BDulJhE2hcVccX6ipiQQ7lerTjiiLOPHFRVIhFqFpFGEcgQlEH1lxMc2TxkVOmycwPkdaDJDyeMAoEWxFRkhB7o=",
            "T00000LefzYnVUayJSgeX3XdKCgB4vk7BVUoqsum": "BPIMyevRyVoKNoghbcdMZurSNjHES5ltO0BhYMCToDOT4aBlLBu4SlVSgUGZdLor80KuZbu5CxTl9cefeFNSEfU=",
            "T00000LXqp1NkfooMAw7Bty2iXTxgTCfsygMnxrT": "BFyhA6BP2mTbgOsmsQFjQ09r9iXn+f3fmceOb+O1aYmr6qDo7KwDv25iOMRV8nBOgunv6EUAtjDKidvME9YkuBQ=",
            "T00000LaFmRAybSKTKjE8UXyf7at2Wcw8iodkoZ8": "BMpn9t4PDeHodoUeiamiipsS3bnNGT4Mbk/ynGJY1pnIuqv4nlEhVOv1CUZ5JbeNcWV/VNTin3xuvl/sOKNx1LU=",
            "T00000LhCXUC5iQCREefnRPRFhxwDJTEbufi41EL": "BFyUBEG/eO5SomaDQZidofp7n0s0eq/9scRAxWp8w+fbb3CnOSffdN3CeNHzJKYgBBmK5anXtvXkkBYCmW7+tiU=",
            "T00000LTSip8Xbjutrtm8RkQzsHKqt28g97xdUxg": "BETTgEv6HFFtxTVCQZBioXc5M2oXb5iPQgoO6qlXlPEzTPK4D2yuz4pAfQqfxwABRvi0nf1EY0CVy9Z3HJf2+CQ=",
            "T00000LcNfcqFPH9vy3EYApkrcXLcQN2hb1ygZWE": "BC81J2PldKUM2+JjkgzmLWcHrAbQy7W9OZFYHdc3myToIMlrXYHuraEp+ncSfGEOkxw3BXYZQtAzp6gD7UKShDU=",
            "T00000LUv7e8RZLNtnE1K9sEfE9SYe74rwYkzEub": "BF7e2Et86zY3PIJ2Bh/wgxcKTTdgxffuvaHJ3AbR99bQr9jAgUNKCyG9qbYDbgU74eUTDZFcoKycGWe7UF4ScFo=",
            "T00000LKfBYfwTcNniDSQqj8fj5atiDqP8ZEJJv6": "BFFVnheBS2yJLwlb+q6xH/DL+RotbvRdd9YeJKug1tP+WppTdB36KzMOHxmHTsh5u9BKgPDgXppFvyBeqYUxoTU=",
            "T00000LXRSDkzrUsseZmfJFnSSBsgm754XwV9SLw": "BDL1+u+QBTf15/susP8JHAr0cbrHrz8iXRnLfZ47izaFtc1ZGhD2OTuCEMUNO0cQC0LhnvZ6QhkaiiPuPb6tC58=",
            "T00000Lgv7jLC3DQ3i3guTVLEVhGaStR4RaUJVwA": "BMmlycOO/y8Z/MDrCUw598nIU0GZlxAgYX+/3MEi6UvguDfnivjdULHO7L2yRkM9hWy3Ch3mKKyqMvIMG2W+Pyk="
        },
        "timestamp": 1599555555,
         "root_ca":{
            "T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB":"-----BEGIN CERTIFICATE-----\r\nMIIDbjCCAlYCCQD1pnLG7w+yejANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3MTEyMFoYDzMwMjIwNDA4MDcxMTIwWjB4\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJ\r\nKoZIhvcNAQkBFg1yb290QHRlc3QuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\r\nMIIBCgKCAQEA390SsOcffSZv4hvcFtKV15ruczuySIj8erY7VD0s3qyQmr49VMH7\r\nJIxFy7ZD74d17JffqOHc2p3raX7bFKA4X10Ezc+MaszfVgwavvmGSEJY6i88OQ1N\r\nXHhK7aLWSc23l40NKdZrKr08mgWlw3WuIUiNyvjGfXf0L82T81ApmWuMQCv3SK6k\r\nn1YX0kyS9BU6D6TBWiSF6ENFGsNEwj6KoNbpe9f8kvylI7jwRT3/ZtQVE182VnF7\r\nXh3FZWbA4PdD4A5TGrpp7BJnmpi6UR3xWnULUSVY6xki+HMkY/jwQp8eajNEr6wW\r\nDN5CZr0YaA2EyiZLOFQKWCINH+X2NRKVUwIDAQABMA0GCSqGSIb3DQEBCwUAA4IB\r\nAQCP+GTMrJpNYtxMZjTAw8yB+iAAhz62WQsiO4TVEourmhRWQluOhO5g0EjVYLHF\r\nonSSahELcU3I9/dcU2X4pMaOBvQDEQ/Z2t4jwAu665HNUeXbB8XV33qtOswvIVJQ\r\nrQ6CNKEhQkhGLz5QnZ8SQIfrP5TLVl8Oq+qfQiTGTnfZXDTa0+8e/+r/KorFB4uT\r\n9PN5bHUxMEocdcmJt0awNmes2NBf0rDuyLFvA9/8yGdsb3n04gSavP6ocx9gVf6n\r\nSg6m94U0ExdkYVlE5ZenZYhiGHXR46bSWnb8khaaw89aju1QbDcrODspJuzdylmW\r\nC+7rfURXWXa/tPCb/GeDFAuQ\r\n-----END CERTIFICATE-----"
        },
        "seedNodes_ca": {
            "T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+XzANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3MzM1NloYDzIwNzkwMTIxMDczMzU2WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNcDIzcUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAJ6huaHiObK1CFluGd376nwUVxJlXFCYYwrOpapo6zaRrLcc\r\nZY2ySolKXS48JdbzVScq5nNZk0Xv5q8jOIQUlyYHNu9g2EEuUV71fZnOhyjXWuGY\r\nfY7Qvp+r/iCZ9n8fXOJBiQGciuVMsu6we4JYqjRLN6gcQzwuCaiUIQs6eINq3bAo\r\nl5L2IunYCAkx9ZrDMWN3BO0KQOQ9aelNBy2pcd70G7yn1+icSV57ODNQFj4L9ow9\r\nujyiw4IdlGR5XiwlOumNoeKqLfAu6qyiGeJ45ZBpmS8XloS1716N7fDi+A/adeJz\r\nVyoimw9o/Q1y2rUiKML8cFYY4eO+PPLU+55sttcCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEARcdEzpnBRk1RE23dufAjm3fMAa+tCOgTqIIGv+OmyxK++T8UIQOAvLiL\r\nFQBaWmENzoPdbAG49dyovcHz9rxLcSt493SeWmB0+GGElyLNccOKaqikRHmcSG7Q\r\n4Fsijr3U0Ql+bgmMETs6pNKAv1z56rw3bULq48VMQlY388XVAADjXHoElvRxKhp2\r\nABCBKEJq3SgJGS5B9PVbvxtCPYH2Rl2djjBd5rQ5Ez6UZiGvIeyeOd+Moylf/l95\r\nK6ydQ79ZQadyzGrdZ6LG+DWIOIgTPqqFD+I8g6ja+Y6IRUMIm/EPAFjZbrZ5bXoo\r\nGhj1Po08WUxXPs+DUTrhpPQgP9uYTQ==\r\n-----END CERTIFICATE-----",
            "T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YDANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3MzkwOFoYDzIwNzkwMTIxMDczOTA4WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNcjJhcUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAOOXwlsE0tzSkmX815v2v2TxbDFixe133iVH7Shn22BgsKNE\r\nX8OOXA+d6Cye6uSDiMIJa0LpZ+6wsBkgyJKHgnShAXglXJbAhT9/iKCzHEbErJz9\r\nq21b03QFPvxLxudkyZ0G4qLjqMLHHQoGpgH9wlTSKWNixojoFTo5H493H9dhqiYi\r\n2fG+AJaF1NLwX2vrRLYXJt53blO+oduZa/f/l/PP/EfPBreXUfsWHfvAv7zb8MbI\r\nRV1xAjyaoLsEfwLgi1bVMbDY1YNUgl8YYlIIP+nFrGoMc+BgXf9i4xSF8Ay5g4KO\r\nH/2ZoLof2PvjUuf2fxSZnV9HSc0875a4/mNgDjECAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAwyJ7So4cqZpR2k597R/sfBffXhbPSuKjiVpWj+cfnKk5ycLB+iU9HpyP\r\n8MlnVX0id9gl7vva8B2YBGBNA0gbww6Sn+ITpo9XUn5mLirPLmUwdqRNNgONzTTc\r\nvtiA5TDLrC6H8gbDcEMl8+eszxFjZ1OkVPiATG7UdIbyXi0WLJVAt0Uq23YHD6nC\r\nm1BgR4S26V+JEjcVAYG9XTn2fvJce6MUULuzwlatwqeXyp2dumGBE6bcrwnci0Xw\r\n75zHMJw9TNXT17S1u8VEfpeqX4U6a2AgmfdxP0KhS/IZSnYFY4GUhxWpVRA4ycoJ\r\nHHAwdeqx79Ru6K2jk5FUxDBQf9xkxQ==\r\n-----END CERTIFICATE-----",
            "T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YTANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NDMyN1oYDzIwNzkwMTIxMDc0MzI3WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNcmtTQ0B0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBALAIoWsnyEc3xIRTOSMbWjQq0mqSVjQCsbOPda1VmK/3HDFe\r\nr3y4E5Noy7jOLhbWeKHFdVIB0wDm7fDDes06yyZ/xiGTPOROYS6C/jolOlQkafLh\r\nBAem5vlKiPb8U3sZpvspoYIxExzyHqvLLN0j8rONnsZSl14X1WAarA+1gOg9+ZQc\r\nvQB1Bvu4yZxpaNOiZUPBbDsj5JfHFOM5NaN/jPHmUluXBydC2uzomYObSUB/2P+O\r\nE+eNMPoITVF3h4HsynxdmrqEcO1OZ4jACZTtr9MPKsrOAfQ17DZ7Gprx/T8d+4Gn\r\nE+cJ3L9Woo0+89SlUOrP8gjykddVNVOJuiCzet8CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAYDu5KHTgXiOF9FHOyLaoRDbddmBSAEAXISs8kEw0sV+Sihvj0Cj5FTaL\r\ncDgq8dXR0m4p3PY45yV652ZWP3Nn+yMvjXFQPICjeQo/Amr85zyflrM1TUo4HCex\r\nbYCFhCIsHgBHopvMsHR1xnKeqTNp5DeHlvH+U5j5qBdCiJG2LZbfOllF+7kg+bSn\r\n+1eIxFgVPEU0BIABOc5gDLhU0FpjAX+OOscmaEiSDs3XZ4yzAytPClwnVVSLmKLX\r\nqbIbYNUoZeSVYgp4+oFTI9x77H9QoK0YoeYRVbS5c3C21Ml94iTV+B2rkh1RXxT6\r\nWzYeGbIPIKhr0HHBMEnMNbGd/zrrKA==\r\n-----END CERTIFICATE-----",
            "T00000LLJ8AsN4hREDtCpuKAxJFwqka9LwiAon3M":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NDY0OFoYDzIwNzkwMTIxMDc0NjQ4WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNN2FDb0B0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAOFs76bhWxDBoUEVioDbQCUvRrRuZzugj76i5Mwvm9azBZ3S\r\nJvqdp4BNCjLc2LKHs9k2bcVyz5YSnq4R0ITC2ozXBkvlusqiihfAUPDT5Yhvwyml\r\nHFbz/sCfs/PHm2NvbP78aKlz8ewxIolnAc4ReXd3zLruh6a9f4m9nt/XPNTbw4Z7\r\nBs8P3qHOE03OrfwlZUjrV784t1BJvD4BETS86dLXnFm2m5nw6e+lIgr84rvIyjOZ\r\nXKeUcvagnPuAcv0cs0yF8u899EHwrrGZX2ASq2/zXeL9GEMRAw3bMhYKpSl0lOsd\r\nn0YRp4btLxnvPGQ2SA9o93iim1bLhZu3M78xNt0CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAcC8BuAc1u3h0+R09ksVsSSM1R4g5vwl/8/tPRCn7LEk/RvM30WJGtnhy\r\nc2zVB82eYQH0ZUl3sYEvco3Sa/4RUerfHWuhgj+aZhLYBAaa9yBwaFzG694JjQKZ\r\nnIFst5CYfvhOs2S0WKnIqD4uikAiVEBbN5qqpLiWlM8b7wOROw3FnhTgEr+YQsOW\r\nYvcc+of3uc9T61nRoUit8zPMVmlsFYlyVuc7qWNU/gJ63MyrqL9iSZ0sG7OTxErX\r\nNzEGOxuiq1Wiqxg2HRBWnEU1gEd1RHlWtPeASSo58ErhU7GeqIl4YCH1qzLneIOB\r\nhjyVJ+dWls+yFhtCUBFkPdTRIwRq6w==\r\n-----END CERTIFICATE-----",
            "T00000LefzYnVUayJSgeX3XdKCgB4vk7BVUoqsum":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+YzANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NTQxN1oYDzIwNzkwMTIxMDc1NDE3WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNYUdTV0B0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMXWcRwHeB/VvycxX5dzaE+XtiyPkaC42i7++RYhY4detUvz\r\ne4o+dpcomBzv1kL0HoxusXDCAZxmVYMJPycVg3x2RMSnHqONlyNr3JuE4Okc8gyq\r\n6IeackkVzKpc7qv3CyZ1v8RPuRSPF5qaphEGJ5N5fKmXaYQ4TJlqhWt0p1NEqAh3\r\nI2alidkEuAjffZxaLwmoEpv0Rp63IVvoQJ6ohFSbWRkwImmjtzKUnfbxIsT5Kp4o\r\nljcQXDuPwgx6gsmnCvruDmQpFMBxFiWqXbwP+k+NXj7623juGLVc63/FK26ZLOVE\r\nP6riClDMvYMBf1BwChOBx47bAigrNx5CBodmnhcCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAZMKC+9xNgVVd6XaAea7O+/ZP/Aw0CDNfmFIXp5allgIktzrHKs/z/xwW\r\nALsJZ4wBgknkKcBJ5Dwtu7I90NzkycKopdYyc5LaI0yo/c6/tV7gD64ZLXixbey2\r\ndjO8Tpp0UtTzRwY/a0MoAvUa/aoM9wXlh/K1r4fWl+QOjkObkaNx+foQEbJrMxIT\r\nTUe/CfJ0qyihE7sIE+u/H02+2THkdSWbE1ozaflRSHMIc/9fxzb1B/0a3NiLC66x\r\nOdqMdEJ3fa1eOXRJXwe0BIUZLirkpiYt7UxP+rHGGNu+lrkWYCvolGoYqRnWVXmO\r\ng0Btsxx9eegVjE9nT2SYTmqHARaw6Q==\r\n-----END CERTIFICATE-----",
            "T00000LXqp1NkfooMAw7Bty2iXTxgTCfsygMnxrT":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZDANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA3NTg0NFoYDzIwNzkwMTIxMDc1ODQ0WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNaXZSYkB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAKoS8vN3pyUis0iQkfA/ZhRgJcGVh+SPohYg9slabsxab1/j\r\nQtubyhjUYEzayHkpOSg6odNhuEuv8gBQxnDAzwThvGMIvrAGnyOTFZjRmO+hpUf0\r\nZL7pV/0ksH1ctBnsgHcSeqR0oUm2eFE8VND7kz3kgF3zStvefj7OWbgQ6CkZT300\r\npvD9pfx/OdXc2J5XeIe6CD+gDgC1aa7sd/wP4QUavJ/iFvufintoBhZ649nQ4TPb\r\nIkPSCtsqhhAl8sQEJLuNIOXNeJCxOcEUr2bFDpCA1JovSPp01rU9iq1XN099Tb1z\r\nFaKmCiOsLqmUl33+BGxCISvylKisqx6d+1yorHUCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAkmT2W7mJ/x7xPY7+RApnAIJYJ5z0UQQx44AXdUDNXg7fgqOTiaNrf8PN\r\n8m9GdpFV4B83vyTf3GzIT64IG5CRupUDQTY/9wDlrQsHo0za4PZE2yQjYesNDQbo\r\n5NSBZ326nGXCIC6cKw/173OuXqptiaqJr9BZTFpG2m16vjtfg/BSkPj6BJp6dhKn\r\nXAf4R3Ty962z8dilhX0xA59XaV8aen32G348S4fLWOnw/oGr3+LdKSYXq6L8oFtv\r\nlhavyQEWu5hGe0+1driq5WkzKhGQG7H5i4bWWCEYLnjwzGcLLJgaD7iOZ8L80V6c\r\nhleJ2esQ3TxVa42Nixvvl9hjxyIJWg==\r\n-----END CERTIFICATE-----",
            "T00000LaFmRAybSKTKjE8UXyf7at2Wcw8iodkoZ8":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZTANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDIwNloYDzIwNzkwMTIxMDgwMjA2WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNSHJqZUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMmVIhNcKMm73/HznUm6bGv6IU1sLqsSSh/cbwR+lM3oDpHv\r\nREGAMIQhwTZkznTJcpxMukFgwu4PQUcD3SPzyII4eE3/Nl1VBtQOFVNIUSLBUXfh\r\niNCwippeEzQf1LFiK9ELyIPghgNymvWViR3IErQLop8lIup3xFdmtLt3+E3X3vRM\r\n8z2v6mKUSpWMAQdWPhT3VwvJswrfRjoOZAXbeHSGoxx+YTiZnTBqMCtEI4+ddx2g\r\nCWaZEk0+D0ir509Ddzih3q4BSsZkJZvC2mx2Vjl4EFO32f6oXsAunkqamUD74gks\r\n9M3iu+IX3k8HOeWznjI/bUmW3TskDB0Vjb1G9FsCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAZvBzXzeRt98nOezS+6STh7ePx9g6++0onyqJyv1nKNQvfyzQQ3a3J45H\r\nblkeoy5qsqtWPlcPbB73juaCaNdkUp2SWUORnQ9CV7AQGsg/MM01/PNKG2eGplsa\r\nm2Q9uSxxXfhG36V64rWbmzOYByxzDtz2cVza/wYGf8vMFcMIMchrzvX4eFwr3aee\r\nX8HZw4DOyfemcGHjapj9v0xsXaXD2B5HZ4sRhdkRScb/pJZJweCoq1Ej8/+4TZ40\r\nZGER5ySlBwZ2e9TLp2TgG9MYisT1q5BZgekg5ePSKjEp79ibN3LX+Vihed7LsVH3\r\nKdfGkGgiBOuM1KM7TinV4y7TAMTIlA==\r\n-----END CERTIFICATE-----",
            "T00000LhCXUC5iQCREefnRPRFhxwDJTEbufi41EL":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----",
            "T00000LTSip8Xbjutrtm8RkQzsHKqt28g97xdUxg":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZTANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDIwNloYDzIwNzkwMTIxMDgwMjA2WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNSHJqZUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMmVIhNcKMm73/HznUm6bGv6IU1sLqsSSh/cbwR+lM3oDpHv\r\nREGAMIQhwTZkznTJcpxMukFgwu4PQUcD3SPzyII4eE3/Nl1VBtQOFVNIUSLBUXfh\r\niNCwippeEzQf1LFiK9ELyIPghgNymvWViR3IErQLop8lIup3xFdmtLt3+E3X3vRM\r\n8z2v6mKUSpWMAQdWPhT3VwvJswrfRjoOZAXbeHSGoxx+YTiZnTBqMCtEI4+ddx2g\r\nCWaZEk0+D0ir509Ddzih3q4BSsZkJZvC2mx2Vjl4EFO32f6oXsAunkqamUD74gks\r\n9M3iu+IX3k8HOeWznjI/bUmW3TskDB0Vjb1G9FsCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAZvBzXzeRt98nOezS+6STh7ePx9g6++0onyqJyv1nKNQvfyzQQ3a3J45H\r\nblkeoy5qsqtWPlcPbB73juaCaNdkUp2SWUORnQ9CV7AQGsg/MM01/PNKG2eGplsa\r\nm2Q9uSxxXfhG36V64rWbmzOYByxzDtz2cVza/wYGf8vMFcMIMchrzvX4eFwr3aee\r\nX8HZw4DOyfemcGHjapj9v0xsXaXD2B5HZ4sRhdkRScb/pJZJweCoq1Ej8/+4TZ40\r\nZGER5ySlBwZ2e9TLp2TgG9MYisT1q5BZgekg5ePSKjEp79ibN3LX+Vihed7LsVH3\r\nKdfGkGgiBOuM1KM7TinV4y7TAMTIlA==\r\n-----END CERTIFICATE-----",
            "T00000LcNfcqFPH9vy3EYApkrcXLcQN2hb1ygZWE":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----",
            "T00000LUv7e8RZLNtnE1K9sEfE9SYe74rwYkzEub":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZTANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDIwNloYDzIwNzkwMTIxMDgwMjA2WjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNSHJqZUB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMmVIhNcKMm73/HznUm6bGv6IU1sLqsSSh/cbwR+lM3oDpHv\r\nREGAMIQhwTZkznTJcpxMukFgwu4PQUcD3SPzyII4eE3/Nl1VBtQOFVNIUSLBUXfh\r\niNCwippeEzQf1LFiK9ELyIPghgNymvWViR3IErQLop8lIup3xFdmtLt3+E3X3vRM\r\n8z2v6mKUSpWMAQdWPhT3VwvJswrfRjoOZAXbeHSGoxx+YTiZnTBqMCtEI4+ddx2g\r\nCWaZEk0+D0ir509Ddzih3q4BSsZkJZvC2mx2Vjl4EFO32f6oXsAunkqamUD74gks\r\n9M3iu+IX3k8HOeWznjI/bUmW3TskDB0Vjb1G9FsCAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAZvBzXzeRt98nOezS+6STh7ePx9g6++0onyqJyv1nKNQvfyzQQ3a3J45H\r\nblkeoy5qsqtWPlcPbB73juaCaNdkUp2SWUORnQ9CV7AQGsg/MM01/PNKG2eGplsa\r\nm2Q9uSxxXfhG36V64rWbmzOYByxzDtz2cVza/wYGf8vMFcMIMchrzvX4eFwr3aee\r\nX8HZw4DOyfemcGHjapj9v0xsXaXD2B5HZ4sRhdkRScb/pJZJweCoq1Ej8/+4TZ40\r\nZGER5ySlBwZ2e9TLp2TgG9MYisT1q5BZgekg5ePSKjEp79ibN3LX+Vihed7LsVH3\r\nKdfGkGgiBOuM1KM7TinV4y7TAMTIlA==\r\n-----END CERTIFICATE-----",
            "T00000LKfBYfwTcNniDSQqj8fj5atiDqP8ZEJJv6":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----",
             "T00000LXRSDkzrUsseZmfJFnSSBsgm754XwV9SLw":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----",
            "T00000Lgv7jLC3DQ3i3guTVLEVhGaStR4RaUJVwA":"-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----"       
        }
    },
    "zone_election_trigger_interval": 5,
    "min_election_committee_size": "6",
    "max_election_committee_size": "8",
    "cluster_election_interval": 17,
    "auditor_group_count": 1,
    "validator_group_count": 2,
    "min_auditor_group_size": "4",
    "max_auditor_group_size": "4",
    "min_validator_group_size": "4",
    "max_validator_group_size": "4",
    "rec_election_interval": "191",
    "zec_election_interval": "111",
    "archive_election_interval": "13",
    "auto_prune_data": "on",
    "http_port": 19081,
    "grpc_port": 19082,
    "dht_port": 19083,
    "msg_port": 19084,
    "ws_port": 19085,
    "chain_name": "galileo"
}
)T";
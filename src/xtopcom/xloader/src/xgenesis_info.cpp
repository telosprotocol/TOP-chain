// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xloader/src/xgenesis_info.h"

namespace top {

static std::string const g_mainnet_genesis_config =
R"T(
{
    "accounts": {
        "genesis_funds_account": {
            "T00000LSmt4xfNdC2v8xQHRSbfSQ7b5aC1UTXWfo": {
                "balance": "999980000000000"
            },
            "T00000LcM8Pn37SRF5RaTHZLsRW4wudDVE9BAXy8": {
                "balance": "1000000000000000"
            },
            "T00000Lg24i2TweBikod4UJwmMS8TqgwvHooFYMp": {
                "balance": "1000000000000000"
            },
            "T00000LcvZaypD3bHFHHh4PsXsy5ASDGAD4mmpFr": {
                "balance": "6400000000000000"
            },
            "T00000LbYrakRkedZqwjdiiqXD5NiyiE6hSGFKhq": {
                "balance": "600000000000000"
            },
            "T00000LLN5NYKGNRVTG2dzpC8o5jQK1Q9ArHDMvL": {
                "balance": "600000000000000"
            },
            "T00000La757CbMh1kpv19cFsCz2CAqVvk5ymuryD": {
                "balance": "600000000000000"
            },
            "T00000LLbE1gpPGbWEmtM3i9DPU1kFC5BzHp8iAx": {
                "balance": "600000000000000"
            },
            "T00000LQVpfcUeqPWmdPJz4sgin1gqDHYPWjRXxE": {
                "balance": "600000000000000"
            }
        },
        "tcc": {
            "T00000LZiwnEtvrRaxEVNoZxU6mp1CFFwvA3JRhQ": {
                "balance": "10000000000"
            },
            "T00000LS2WKtMuE7moMekoiYhk78rdFYvJzayMDK": {
                "balance": "5000000000"
            },
            "T00000LZkFxsyiz8nJuruAzdtvi8YhZpmL2U9dXK": {
                "balance": "5000000000"
            }
        }
    },
    "seedNodes": {
        "T00000Lgxs8TNdp3rmGiWrs1vT5tK8YfnngmYMA7": "BKr1oPAEqKBEULcFfg6UGhXpzmeZ8EfRbX6iD83FkrF96V7ixAoUW10SoSyjRK+4VZiegtN64bEmzNcEnp9QdCs=",
        "T00000LXWJsPAZpNt4dwfEk6UFZvCLXtcB31PAYu": "BOvoyESj1QuNYwsN62EVsBHWEPbUTU4jla/74VJGn65Yvrko21hULS5LAf0kjW+OoE1Omv2sPcQhCjEYielua9I=",
        "T00000LVBxDD54YM4E2nvn6beUDF61YsCzEuRBGY": "BLSkexaTXEF/yXYxsTlKveULf/qotzao2f9r59WKxhtsYR+CcjG7MHqSDtiS9rZQZ2oFBnwnxTlAAAuhV5weJL4=",
        "T00000LLxrHeJwtUJZERK2H5u3dnwkSkvg7nPH77": "BJH6sBaYIn+qU5//dTONqbJ1rTtx8E/F3rNTqem1EBAkk/qIC2n70xAxwHBabRqFepb9f9Xjm+hio9h+9vwasFE=",
        "T00000Lg6TGLaSjcW4osf4tUWbBR35AJyxDE6iJG": "BGxD3R4bTza5TMjLjRToG86UzMpSzOPwta4L3nQKwi8R/PXyOjZK62O0N9twZAHtlN3B/Qh7N7xXFfBOB37j5aw=",
        "T00000LYX4icmJR1DGjLgsuNWX8LCLrCWxi5vxa8": "BJRU822wjD/LTH/HDrR9bSyfRXneWzDe4gS/EJfURpggh9G0IsV7gAQMEDJkm9Nm9CsLFmA91SNy1oRdysPoSl8=",
        "T00000LhjvPYavSzZ1uhYSwMgBbGGJYzSDvBdboc": "BPj5qjAAOdXAkzCzUbDfXE3rmV5JF2gzb2tPStXGOqJm5yatL6xtPZN/M7t+ojJMH+iZlsVB5t5LLcYmEZRwjyQ=",
        "T00000LNEYkMfkBfzwJG1DWiCS6nZfXiz7KGPjap": "BMIkX1cXkYdSguSUcbDL3jRgsheaezQ9zgmkQzXysdg/DU3uDFM95XjLDTl5icc2ZVsKW+K5faU6EjA4CPxAmeM=",
        "T00000LiRhS1tpkgZMeHcgGpvcxdzPRXpjbFYXBm": "BEyD6xvaXh50BaMkGAP2nlnii3gGTrosc75MYwlCK6pj8UDv5ca+T0x7VEPBMo6neO6xwH/TMfQnRDb69Qoe9xg=",
        "T00000LMPkQAUgSJSTaGFUkMvUoQ6mrCuPpGUNVA": "BCDZnrix6GlJsCwul6KonTnAi5hpmpSazyHGqM0ImCZpgN+3b/owue7RD9MkCp4qq1UqOzXlhM0/bc0xFLGQkto=",
        "T00000LTBong3HQuvaSHVTFnfLjuutsPcbZRD7LU": "BDtYMlbSDkvs10uASnmZbItjsjZljvzYCZ0TefriMfNMTtc+/9eA1ChVBYq2kd2IvMqFWZEV+UszkGPgHeKir8Y=",
        "T00000LeaUsjhbNCfcFk2RJc4HqXZ5QkyUCWRu1T": "BDs9PTZnEpRvhVhU3TVYcIqBOLVmdYS/4X7NG6O6jwkDuuMbij9NlFOfFeW00z1iA32td9JJaqh2JhNvTtJWyAg=",
        "T00000LKMJveVLwf6MBGGMwRYSgWJezJSkt6Bzrt": "BACma8DvDibHx2XsyB6ENDiJaFXhcrEqxFsoIqYoV9zi14RFQ2XLi5QVYkR9F+t8tgEm6UCGIp5EwuoulOklEAY=",
        "T00000LQe3HJJnLtTRtdhzsg8Xr5eAajDnDTt3Hu": "BAHeOAXnBqGIbhvx/XP+7Mo7hMG2qDzwc7+81AMO4KAKXnAoLVDBXOINvkG41Ljo0w3P0cpK1P6xoHX+7BLKnNE=",
        "T00000Lc9dSbHz9B9aCsRiXvo28g9AkxsVa7h75R": "BCE1Kzxc1ovPPQKN0GTHlEl65Y+UEMbhR/ZIkxXidPG0q6ACqOhwZphe9qohIX6jGEeppRcSctXoqpgFh9IJz58=",
        "T00000LQKZhE62hKoynBpMmiBSQLzZa5H5bxuHvc": "BL24fZKwtroJGvdZMvqNLmO2GWWpBEAOtVIDwGuQ3+yEaSYcJ5mINrDZBxyLMY9KgzYCafnpn9U0pSC9INhXzM8=",
        "T00000LdcBqZweitHrSSwFR3DNBxuhDBJ7tWWFdD": "BB+pspPGmurmHwYxLFSxsLrGbdVsGrT/8tcGU8Mb5LrQveoi7d/8aATtwZwrsBdxe8tNXjD2hnAWG8vqcix9E/Y=",
        "T00000Lbjn7Y5NgQRLJakXnizgtWpk1iCGwxp1F5": "BOWmPhQHtYnd9TNOWgLSlFN0XVV3PPzwJeGoOo+ZtGSu1Fv58p9dUMaKHYeXoGWOYCkdpOgzVHXA4ZMY7uRY2U8=",
        "T00000LgdfinSxDoUWzDxiB43ofXw2wtjhb5dcQL": "BGqLJdi3sYW8fOwySDX3U0TGN2oiB/kh4jgYe0EcDGFSXFCl6BNMYeT89b723MKrrpPGJMdGMSe+7LCINpFt+OY=",
        "T00000LZGXDjMtc6E4DFqyfK9o9rmso4xW2eA3aP": "BAXdm513bIGd0nIw6MOYmcghbFFDsZlBhl085xEyDGBkJ+d3jYUo9YVaIgUk5TnXTP7d3tZ0XmqCtvXKfYZWN64=",
        "T00000LT9SREE6X39ieVqEd9d9n54Lwceeu3DP2d": "BCfmX1ULfx+SQfnt0RrhfA9CoqCESBnDjn1+YWJdtBrQsQaX5ycrhEFvGfCQdSaJ1mD8fnrDVC25ETf6hxr/+CA=",
        "T00000LcfLSY9rFe89FMTtcMu2kZDpWNbzaYYFjY": "BCpC1XNevLDDCAdZLi1mUtCY3ZLx2pu7VNXxCQrCOPr3mNyFmAK0H/xoJn5PJiRnQaSZpjIM71n9tcTz33MQwA4=",
        "T00000LUZbiJt9PQGZDo2yndk9VxDgD6BtWFjKqn": "BI5Cg74mkiB3XHKFMESzfRmUpTVdKEh981ln8AE0MI3UpUAQQZQbXVec0OGMFIRUgBS6WFPoOLDsDgA6J0Xskxw=",
        "T00000LdKC3iNXw7Tb4JfRUfzcrcNg16xnatjymZ": "BFDKXJCY3SebmEnX9bn0x3Jktu3UOdsBMY3F8P7DQNRtLe6tamxXOmDPQv2BM5YNqCsJ41BKgvSh0akOYfCVjuc=",
        "T00000LU2MFB41mxPFwW28KSQGsg7PrEZZMyaPjh": "BP7f/jQtxS61B3E0SYJ227uZn5pUqfvOoT6PERNUqAYwfIOfxBoNc0s8ASUlffKP7AOqV6COOtVGMWp3WyY5nRs=",
        "T00000LgcmtYdPkhaN33Si4wSCzeMZkW9gg1JRzb": "BP0/ujE0oMSOIjqqsj7VVrzX3TBtWB0nL58s6o73wz5cPPAD0X+61y+NUPuRq9wPIdnoANQCyAt9F+mPHqkWCGE=",
        "T00000LKxhqjopwc4W6f2KVjMEpa4XZVtP3qndwW": "BLcEYXR0P1FSYdZCrx0DxcwnepTGJKQayqtT/qZeoo5cu8uWP8I/3FObXMNm66D0mvEkXZ65GiWcNT/tsOuEN44=",
        "T00000LcoX55fYMmhBMk9uBw9UPsGM8UEFHMEd43": "BCVVXZA93x3ViXf3sjzHTfkjs2NyZ3P7C3Jjod8eeJ3LpQFMUJ+ZnVapioX/6irj9zmow0PTdbAOtKG4cy+T/5E=",
        "T00000LWECagevpw7GhoaSFKFx9RnM8TK2eaLcWZ": "BEmYYO2QMSEtAwxNnnHRFA5dDwG9tDw1r1pJ5Dvp1rfCUaDm5WuyEeKUDaBGzQDmrlb6J0J7q5IUghELgw8/B4Y=",
        "T00000LfTfxeEhXPhfkGzjAFJG3AeEATW9FwevRv": "BK5HtxXcLU9kg8AnTdA934uJPMBjuWasnuCpkq0cMzLp4nh106CJlBQkg4MUWNc1gVzXopKKKQXBH9Q9qR1q1JE=",
        "T00000LRv79bSmyrxvi5taTTMLf7FkXMQuxjyn6V": "BKiBCbDqyNVc1EE1x3A0dLvtgTDCCs/XmyTWNcC8FTSXg2N+HJZOhfNLK679fc4dJ4FtWfxgduRImt70gtPMgEQ=",
        "T00000Lb6GyxbFpseNPt98rnP7hN7esEPZFDhhbg": "BHYq88WjvI2Gs7H3tCXR9gktR/h1uRI93uZay1ZAm8EVE/4zL14PorB3IQtuiUwJG+VmgycfVIKxELrpaHq/m3U="
    },
    "timestamp": 1599555555
}
)T";

static std::string const g_testnet_genesis_config =
R"T(
{
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
        "T0000013eh6AcbYg4b3MZiSGJFe85Hx4hDU9RCpj": "A1+X0/Ut/Adz2NZjg57RveRpg9ljQsR5t5vrQ6SYE1xx",
        "T0000013rDnLF8mkM2ua9MLtMYCoBk9682tyRimu": "A6TT4JsYAhtr9c4W2fNXOQCBJc0+9K7nnWPIV3oXs+2Z",
        "T0000015jgbeuHRWjD3QPAPhkWC3Y5xQwYEL8HMd": "A87W3lN+anWdz0BxFpTFKiYFEageuAIMp8Ae0kiw2V2o",
        "T0000016kdzmUVZH4ZkFHJfzTcdRmpc1SkFuNyMx": "Asw6d5H58J3gL2/E5PjvplwTkWUEEC7w4pOsYmO6lSBz",
        "T000001DNG4izekAUndsy5dAtg4GfY7nVvxmsKbx": "Agm/ekUB2DWkybi4Ly3hn3RJEVvgikxAox3wHLmvBoyX",
        "T000001JkyJSZTdn2UozFqn1odFtNgqzaswKHBiL": "Ay2bSqZ/HyrrBud2pj4xTyfCy8zWIcjFWxQ0IlTcF8FW",
        "T000001L6JpJaBKvVsHrWnnUwLAbj1876ZuPSSgM": "A6FA8EeR5BzZvctzz2FKDcTIWnlhxmrUa5ZhWSeW333w",
        "T00000131d6BERLzV7FnobaCpgEjYuYDQ7R8NAP6": "A9J4uK2TlJwnF5y49Rrbw6EjtGBNgO8MZmlsaC4d5PU3",
        "T000001Hp6DXZDkWj8bS39NsNuNMz5rr63S2pBza": "A26k32RgcRUDwUcELyd3HypRtZinooPhZ2UE4frqFMIu",
        "T0000015GwmF4Xkncjq36BE6PbVYFUEnv8v29U8Q": "A8uuVgzP2k98t8Dkbd2God/lk4D4+SrQAs0S974UsFbQ",
        "T000001KZqPrcUtRaG2nCue4uQMFoWaq5Sasja2": "Ar2rAjlQ9RIbiVk902UAuxi6RMKBnNKO3+ySEBVAEGZS",
        "T000001KbmrndswCbfmHEM74gvZP51FwDAFS7NSB": "AkL2TS9RCWgLY2YNMOaEip//prt08GpK1dZz9FiwGYor",
        "T000001L935wdv4NYdM6UuT6VqUi9jgFuR5QUtjR": "ArfMvOaaTPTY2aDu0TBa5itjdfi96cvWJso2k456hBBO",
        "T0000016Xm6iKhaorpB39LpNHCXJCaTE3YzX8dwG": "AhXGwicjFQG9PXdOcLgzSAb5JkwX3k3Od7FIv44dvFNb",
        "T000001CsCNrw3UVYBBkjxWEzNBfo4Rq2MufA5UJ": "AtEIFmQrJovGvGYymQN12j6tKuNERIDQttKQyQOE/P25",
        "T0000016pcBBWTDJU97JWB6Tnq1BPDdQSkk6jN6c": "A+pvsWy0j6QJnzrsYdsYCQ9KhgQDpECkkLW3CPvCRzLM",
        "T000001CNtsScVadmpB4ZYjQ8iqzKtnAzm2sCSdd": "AwX1G4jH20OrrwbLXOMr6Fyoq0M/xnmLDbvpU8WTI/4i",
        "T000001DGVoAAopHibS1xqogPGrDwk5obs3C5dGJ": "AhzB+X4qKoIKARXdGEkBt8XTu28YUABNSRdOEnEe1biq",
        "T000001Lmyi342K73wSLg83d9gH18g7qmjKQNSrR": "A8vn56fHIPONnkQvQgFRdV6gNrcvi4d7/ACGmFbpZZ+q",
        "T0000012zBtv2LjzosXXVvrFaenApN1r1XJUeitE": "A+Co9+OOopYexEMDgT9r1NDpCHgr6QGCpYTVK+3j2fOI",
        "T0000014BMwxmqcqHvDceNqeRnSEgToYXieEjnKz": "A2BA+I3t4ONw0SVEOoQguPiSp2MeNNDzNi2ae8e802ZU",
        "T000001FxxYfDeUWPUFZ8kReaUxJdpoxKiaZc2T9": "A4T8p3TLWEX73oTWaG+OI+RL81xvycNqM0106OWkoPiJ",
        "T000001MsgcCCNKJ48TfrAdY3egeYdV58v2XXvv3": "A9MwkV26uTRqn6qY0Rjgh0kG3CvnwF99mf/ZRCamGY05",
        "T000001H9VJuHbYua238tcuHkDgTencuv1MhtNK2": "A3kbBUUnTjGV0iZiF1YFoj24PK2RmSlVSm+Y5UbyZsn8",
        "T0000016xCd3gv4NiWzrr1hxTFSPFoXYqYHwvGJK": "A9pWY+avClRQEIegF4nXOuTGw8ubowAhHtlrvBNa4S/v",
        "T0000016P46V1q1gUMzvU7ZSDjuXKC88HmdK9Ssu": "AyAqz47uUhR5Wv1vkZ0MiNvCSPJFoOWMDmodcfXa4ySj",
        "T0000017mqXU2X1QPnfLsLZHsm4YLRWcbfvwq7BD": "A0MNrA0PTWKh50Ot1JGdqxbwF14ugOHqHwe8xJWeFjwQ",
        "T000001MPh2GXgxMX4SMi3Y8bw9QGPY1YdN9iunj": "AgXK9lfi6p/CEr2GtOwxtSFAS/pelS8puA6gZdWW/L1K",
        "T000001PJZWAupW6FYpYnHaeum87QTi13nbboNo6": "A2XvJ+mTPCGArUSoCK5As57N9CfPlPS4JZLAgXmJMyFh",
        "T000001MczJzUFwyGUPNuUXLSFyJsfQjRDL64s4q": "Aqi+ZqQ6vprCfeUwJuC8J6wR+JR1mTc7gs4k4Jlr1t+O",
        "T0000019N79oLSm33ZSRGzG6tBnoodxPdiqLasCb": "AlFI4TvQ7R56BmyBmvxJyuBZmCmqmkLgz6hoPN7IHDgR"
    },
    "timestamp": 1599555555
}
)T";

const std::string & get_genesis_info() {
#if defined(XBUILD_GALILEO)
    return g_testnet_genesis_config;
#else
    return g_mainnet_genesis_config;
#endif
}

}

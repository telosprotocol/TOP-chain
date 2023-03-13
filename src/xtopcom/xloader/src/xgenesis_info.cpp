// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xloader/xgenesis_info.h"

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
        "genesis_funds_account": {
            "T00000LVgeU1bqasm3TPv6Lm3bKt5ZhqUsnxMqw6": {
                "balance": "999980000000000"
            },
            "T00000LXJUq2ESPswHemepjLAnK89qCa2j2d2PL2": {
                "balance": "1000000000000000"
            },
            "T00000Ld7MbJzspCG7Etzq9nPt1XaJHxRU3h2Sto": {
                "balance": "1000000000000000"
            },
            "T00000LfsZM361wz9wAWxfe6ziWhNeFQdaAnVK2z": {
                "balance": "6400000000000000"
            },
            "T00000Lfq4CyvRBnrGsWQSxDT63S24VcXonrLMMf": {
                "balance": "600000000000000"
            },
            "T00000LX4U2aWyhPf2ucdEE9br1tFMTJpDCUQwdF": {
                "balance": "600000000000000"
            },
            "T00000Ld37ymxWZKJy8Bzj1DNtmLKyNGYGdm3ovS": {
                "balance": "600000000000000"
            },
            "T00000Lb5mfahHoRhgPASrJL1qSiYRepaWrKjszz": {
                "balance": "600000000000000"
            },
            "T00000LbXwNMTNUBu9VewgDfprfes1JhSeuLvmU8": {
                "balance": "600000000000000"
            }
        },
        "tcc": {
            "T00000LUhVfsUWeMgb4Gw8Q8MNRamDubgF7nE7ES": {
                "balance": "10000000000"
            },
            "T00000LiP3v3gUnAGbtajwmCLwSyyWtuiXgUWpwo": {
                "balance": "5000000000"
            },
            "T00000LTrdB2vwrQ96RFSBY1fWp6DD6yb85kvNeN": {
                "balance": "5000000000"
            }
        }
    },
    "seedNodes": {
        "T00000LdkgCx94FNHF3AnmEEj3HLyY5w8gr4skYT": "BHnm93i00YjFJDI8v4lSxjxPctWcCKkHupyL3AJmkH2FI2e7m8wCAvxCY6OyGuZq4BJ9H3R8n27iGk2f0DGYXVE=",
        "T00000LSSi1HPMJKgkuvuzgnD3t7qZsVKRvxjuhj": "BPYP90nOTqCdM1d82XXXkZDdFj6VgzqLNz/lGhvAYIV5TmP8Vvk90HtJmVxiofJ1sqWpfh8nicI2+//Fhotf3VU=",
        "T00000LZoQ25d4ZJibqRhrRQvDT7TcQ5M1Jwwq3o": "BIT0XbMyOr6Du7EZJ4cV/JDe4JhrcYHrLgfW27KKhsR1EE+5E3rJ9Q9J5crAT2hgeWFL/JgRVL7z5xfIMic3Byc=",
        "T00000LNCif6hJ5WKWVvvgzj15ru3WBbVd5XsYqd": "BETd2M3lZ/5iqmSHJj4N72E+JYzU5bXwA4V5tKNGPnRo3rOLXhCB4Ogv1oZQESNztY5GxTwXmTaxejXYGPS9Jzg=",
        "T00000LN6nySYBV9SEQY2j8Qs2cc8fCSp1Q4sJNL": "BLI/KaemiQQtVVmeeTwQIrJXU3p7CrDdoaUqG7jPx3io1l6NHnzEqQpy1/QrL47YZvrwGbbR35HSSLwihJGG4d0=",
        "T00000LhdiM7RdkiZSnrtgsEGnYKena4r3nZ3WCY": "BNVzrR1XWTsf4Xp+pjdrEySi9bjpJE8pv9kWEhnQJmV71eCwu+o3ci3MsGovT+WmNSAtEp+p5pMgsFyWbj2qXn4=",
        "T00000LS5hzBt1ieww9WtuC9Pz9hpbhBnPqBzM7p": "BJ7BTx946m/TVPfNpdNVd2V5jYQhpNctLaTxSgCLA6L0WRlIf3UZ3HhnGIy+lDl3dBnfSYrj28Pz+eSHn0svUsI=",
        "T00000LfTdm2BmPsiSwPJQ4WwZz6mNDfUXPxykjK": "BPRo43Sl7B98zxvB8S5FKdQkh7lllJS8Kfwm2EAWeNAQplnnRqlQKM1GhHSpyY9OGRCVB7wDTZ7QSB5IZXiHvQ4=",
        "T00000Lbx1jRYujqLUNuomTuaS5oXyhATY19eEoK": "BCnn3qJJ9P9M7eckEYhwE1OZu3fuVWwh0grUiFg35yjqy89kQ4df8WFV0t/Y6N+GFRQwrIDMOtX2hcJFnmiqgII=",
        "T00000LZXogjUCL1vAgd9CvavgZAEG1aVkC4wRRx": "BF9WTr2q709MfSJHCkX33nHCX45AWPKlvTKGJhRx3dc+l44BSfW5EzVGy0GkmZaVczKvgLz1kNb0RUKWGHmGR+E=",
        "T00000LQVwfoXXbcXmQ4DRcZ4S7raX44swsrikDz": "BLtIq6G9om6MboeVTDdSm4o7SQ+VPItTLyxcgJUspUoa0wk5xwwbvm6wV/0x+3dRNKYkuXB6EZ66Saqy2q9ZHJo=",
        "T00000LiWChpMiWhmGKzoKPc7388Hjv6StvQWTvG": "BKGDM14xI1hdv5ay26FNneBItTwJGOvSKUwm7fRJM7iYs3Uq/Wqhui4T3SfGhuZ7cXQWIJSj79bbsRmE8hsb6gU=",
        "T00000LWFUoPXqdWcPKhpmAXTXAJ5ibL66Sz82nz": "BOCBO5pFw2gFRZg6zSXB0voMxyhYCMWHM0qOQIT41NaJojC8msQUMEbl3gk/sklwlzBk3c/ZjZI99JOy/jiFTwo=",
        "T00000LfPCU3JfHoKyDpLapaYRdud3kKWQ5NuxiH": "BL5+o5zmv95PzDufP17OQErpF0kbFTZgKL0ZdIlJuDjA//S2PLiAY0ghagPBhJLTqZMEwE934R1AqzqyrGoBuWk=",
        "T00000LUJvBTq2j9xCdqLCX8ryBPio5RPePjEUCp": "BMB6UsuA12OMbBsILCMjkU0WfZEwP1t2aWefN4u6sxXyeb2+Yn0o428+u7En/LVWyqz2Aci+dyL+17g9K0RIpJY=",
        "T00000LcpLysLnuc2gm3qnySvuM64q5jTCYEV6rh": "BGFZD6z/z8xWJsY8p8EMLMcdtqC5fW05eEI4uBzEt8LGGB+zhpZywlgL1HVXAwePLcIhGo3Q5x8vrnBlN+f5Jlo=",
        "T00000Lbe5K4qsFKsR93VBZs1j9c2ytU6h51LZAX": "BMUjiKFB/8lNkW/YSEoJ2zCSMUzlx3ouSjzvKkLPrHjgGCl5kDse6jYIKDiQK2Sii/57cvQ9uymLX2Dg7dZB02s=",
        "T00000LVt1xonC5cxJMryeeFx4SRHK9T6p6o36vX": "BLfsFumA8GwTZW1NISFMLTNo8D7xJgbTGUz0+Baga6XZ6KbQHQwwoPiY1R0cq0PVF2dtkC2x8Fn9UmlRExt23U4=",
        "T00000LaHzQAjz2aWRxCBuLpZxBXAKJAaT9Lo74f": "BIrQ+SBu5MlL093AnhtNM/OQOpeM7w4ZehakDqB2mGqCD4dqN97V04ZKm/13JKCqMbqbzrD6BE8AKG9mcT4EQnU=",
        "T00000LXtozggvP7FobhbtGjTbfw6kVqVgkuU9cN": "BBTWRHjE1qt2Zz5o8eLagOX+JR/OkJJ/fc/G2vDAywtPH/CkLR7wwj+2YBtPmhOH4g4L9++9HmCj65acUJXz4gY=",
        "T00000LUsPHHkSH5uWfHLV88u3CkbJvf633UkjEe": "BIMMzCphjZ8PaPKDtesDad8/ZfdFQpq62SXvF5fuXuaqDB30FeVP21U8NGEN7vsJMpuhbiBY+Oju6FmshMH3DdE=",
        "T00000LTgMnzXpzyLfoitrJpS2QP3rPaZ5ycte3y": "BOumiIVptRyB622rtBkT/s7N3f1aKl6mzm5h/n2nMLzlK9FiBPJrlZ6qLgMgkqynSj193R3FcYytmvVVp1vhNaw=",
        "T00000LhATPpF7Fw2yrt8bhaNQmCRvobAroDNsaN": "BMZCak8s6hvAklHU+daJ2lZ4wIqIOzpjKEPlCzoN/euYYG1dia6maaWS62BXAi0Z9iavjbsP5b4a/RQwFAzqLfM=",
        "T00000LMH32jPPmimRZQSEEq7ef37fR1iHK4ToCw": "BMqZe2IC9YQqVzbi7J2etGgEh/9wwd5wGXNL6TdpEVoGThgUSn7qnHhfKsE/loHSPc6Q5T/PENbtZd2I+0OfmxM=",
        "T00000LfTxMaBGoJkY7ongjs448AcZGYjdy8vzq9": "BGrBGGhkn1vQMVtOuOYEl1LiVtKr+oVUR8aCfWbyQknDCW/hDooWYIevn+XcyMjt+AMxjvUqWAVEV4juELfNXfU=",
        "T00000LdELZJjHcTGauF95DvYf14Bq7hHu9rLsPC": "BCc0DrhxIlmNjBd1IAzHAjGgkY/DiZ6xozdvm5JxJUbfIu8mDE0IhGu7jQ56e6t8fj6g59ZXCw8PkujKNIDiqYA=",
        "T00000LaUWK4rC7iy6kVmbDTLKndPfmG5TMYZQiS": "BPqvbOb4AlKbE0zR4hIg7ZzzPM6MrdsSvmsTWkvPmOAtoLF5MQYPHq/haeSPHGD0jqNxM4bPElL0I5oLbsAgFs4=",
        "T00000LU1ak6FrHXnd233q9e9rKmTmjVjovDD5SG": "BLKipdutq87yTH4ZNgfFGbai7wmxddz36I5hSICg8Vq94UPnWz5AhLsr5ESR+BqZwn2bd72UiyYkrhrI8GVAqJQ=",
        "T00000LNHFETMNpdrmqz6XsNpTSkk4hdK7vHderg": "BKVXRHjjqZCfJbN3YxOuWSCosk0RBL19t9lyn3obEmxJ+GFNHYOWxsl/2yyGYRlVrX4Zz5kuaqPw0YebGTvFNiI=",
        "T00000LNL2WwX4BVGWcegbK5FXRzNZPCvCjPodWT": "BDsGUkHJz7HjgS8R3wnbCzxYvAeg9kalyhkeeIENszPtCg1IAMPwkP/rlCZo4OIEzERCO64BgzF8779pxavzhV0=",
        "T00000LhT2y9CCJYWc47kdLVWZEXAP1CPjx3ukCn": "BJcSuJnX/CUFL9WdU4JFIBvexgVSE2C57FViSKBxvEMscXDNqP3S/fwZgMGiQwxgnwT0fAKJ4gz1GeRnIMtzdYI=",
        "T00000Lc1vvrhq8XLof5LFA4G1JxPpStCvfAj2SB": "BMuHEDnrV5Tz9zzJw9hrgQshOqrtRiqgZGBdDYbjIkYm0RqVaSO6nViGprs2qLKU3pxBAWG2Rc7phvflO7lWQJ4="
    },
    "timestamp": 1599555555
}
)T";

static std::string const g_dev_genesis_config =
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
        "timestamp": 1599555555
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

static std::string const g_ci_genesis_config =
    R"T(
{
    "accounts": {
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
        },
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
        }
    },
    "seedNodes": {
        "T00000LeEMLtDCHkwrBrK8Gdqfik66Kjokewp23q":"BPZmAPKWLhhVDkJvWbSPAp3uoBqfTZG0j2QLyOaT5s3JqOxjIvTQFnmBXNUiMV3xwJ/bp9Sq7vD47fvAiGnC4DA=",
        "T00000LVeEo7QGURbT7Kxc5SxMmMxvbhXXc1r2aq":"BCXnLl94yQR8L1pLdz5SqEhHIkjifycTNYms44y80OrzSAa1zJ7d8umPcccTxiFaRbqPQvfWmdsr560809aX1bI=",
        "T00000LMhJMWwmFMwTfpfrCzqtQQ9DBveVUirkSC":"BMz9dh7/AozzPXzP8vq5jaMKtK7YYTM0YO/C7P4jFWi33kfZo9poTjAceSRJJ5lO8ch+nM71nUuLpqYxgZ+zRpE=",
        "T00000LScQir9BbHc2YW5iE1kVYp6ncYds337aCo":"BD1vW7NQad07ppFtPEuepXjklHzbSJNoTXYBFfM5hD8uRxiwcQJLp053MdVhoI77iB3/NfEK6q//VvP/MNN+yFk=",
        "T00000LV1ooVsoMHfL6MnCDx64Hdg4vchnbfaGSW":"BPDmX32DPqJRb+QzskUw2SpSoRxn8ObCIE3JYl0ZI6rOKDRsVXTrjDDOtWICHl0CVjV0N2Ez3pWiWFiNsgg8Nfw=",
        "T00000LSvT8hqFL39GvmuotQBvksALLcubR3ivRb":"BECRZwpagglNLIjzXvNWqyaTo3pCZS8wT9siQj+ZURPCHqOVd2e6O/G5aRHcm6y6wrNpjzpmDs/23zDYiaUu/KA=",
        "T00000LcFkZzzADMS6tJnzC6XH5N79ZCrq6UHrje":"BLKB1F4P0a9JDzVxhuoZfIjsPa1LgecC1QkoIdepjtBH5mfXVebncbOPoPte69q/yhmL5fBipnY9xg0o1cnemnI=",
        "T00000LTLK35EK52a4mzTCSKxW9BmHUqqNKGoDmX":"BFJiEebMdm8tjC2dB60G0OvnpbKkBhe6zBQ+F9i4dIRtyn6Wv5NVN4NpsDxN+04duNuSfZQ0JxUhPz8e3I3aZAI="
    },
    "timestamp": 1599555555
}
)T";

static std::string const g_bounty_genesis_config =
    R"T(
{
    "accounts": {
        "genesis_funds_account": {
            "T00000LNRqBWTkq2N8RrU3XpsdtbgTw7PVFBwfSg": {
                "balance": "999980000000000"
            },
            "T00000LUrTFLFZtfLzrePDDMkiTUsjsjqSThKBw3": {
                "balance": "1000000000000000"
            },
            "T00000LUyt7eKqPBJXTd1pNa8zrwxXyaXiJpavEn": {
                "balance": "1000000000000000"
            },
            "T00000LhegUboSZQjmaoUuQjWkgEbnWr4SKsVbDB": {
                "balance": "6400000000000000"
            },
            "T00000LZEC1m7T4RB3yvMiYBXzhvB7GmJKg4GQ5t": {
                "balance": "600000000000000"
            },
            "T00000LYiyZ6jwBmeUonYZBMxNnVjfyL31ZPfJso": {
                "balance": "600000000000000"
            },
            "T00000Lh8hsRssDCXD2JPja1jWLsYbXkHv3kwydi": {
                "balance": "600000000000000"
            },
            "T00000LRvasjhTLjRcriUnp4rVeUgiSUf3eoyNN7": {
                "balance": "600000000000000"
            },
            "T00000LKvuNz6bEZ7bjqRmmLLV8LY6vm5fx45bin": {
                "balance": "600000000000000"
            }
        },
        "tcc": {
            "T00000LeQppogKVy5iYYYfgqDcjGYMxjTBDJPo7b": {
                "balance": "10000000000"
            },
            "T00000LewaD7CAhyRKeACYEzZZzkXs2e5ps1D623": {
                "balance": "5000000000"
            },
            "T00000Laio7tt9wBNyNwbFGdRv8EvEJT7ChY8YTz": {
                "balance": "5000000000"
            }
        }
    },
    "seedNodes": {
        "T00000LZvPUVnThcwDibBqPvqXFCQbMQ8ZUGHhM8": "BLrBA8tqQx5NXOrhtZsR/t+VyGxfYruBZDa4D9hCEZgjVU9zPROKwQLJ2ZDO2kcxF+BPpI7JwHrWQ1/ixkA0KlI=",
        "T00000LU1BzFaWchJyJS7GNKv5tD1c6c2DZJDJaX": "BClXP19agNA5u4vdV5x7VLqrOuHTWU4TY8hoI6QVmawbase4E4mRS/vhZUwFSQBPEsfvNDizsqVZL2ru9c81boE=",
        "T00000LSxqwc566LJRfX3Bzh8kXhJanbtH3JfE3P": "BG/iJ3OojSwKvDd0z04pUWe55Rb6gyrrkXxUQIol0/gJrJpLKXM9DtYe7ZakeQr7PyPPEs7Os9ehT4kt1/Kv+pk=",
        "T00000LchSWvdKJ6t8NjYYc638F1kPcST1VmjDa5": "BCXj5NRgu58c85ZAMhLgMQ0eQGCfXTM801VmZg0UmbamLPQBVTedRfDcJ6lg5oTdv/2F0As1gwgjK5UN+N/Mo6c=",
        "T00000Lc1Fdb3vQyBze9rkyM6jtAr8TbjC43U4i6": "BC7VNUX8Iw8pAWx56vGsqS7zlOebY2ccHK5nAO/neuxcIUPTL2PjyyGYeMlTDMYljU7BoQzl40oOw1hiBcdhskU=",
        "T00000LMZTg9Vq4kAym4Zsuc9tATdyghZrcVfyA2": "BK/SqYTnCviUcv9cFvCYE0PQ5EN3vbPLKxWo3jSOKO1LVoz3AfV8CNVdNImWPyZ5aTj5AAxSPk9segTE9ooTbU4=",
        "T00000LdD57YPiVkVN5P4pTEVFWHrnBCYsWEFqbP": "BD+xpdxwfplVIQ0oJeqhuKBGJA1d6hAPScBrUTFhgzgxEfI2XAmSvBYiLEtfb2nXzlv/6B3IYkjAuiNge2Jonpo=",
        "T00000LPwHDcC776mWsSmMPV3RrfY1RkRGL4vS82": "BHAl5pM+0KujgJmvO9HjbZrYDTmmZ4b9Ap8/tOrpA6G3if+r+ILMczan8HQlhfoJ982QPp/vCqSGcnCYTQGGL4U=",
        "T00000LUResaJgGKGfQupS6b1N85XqGPZydhgdFG": "BD+L+Ow8G9uci3cKu+K6z6IgayWgNGHd55KQhz+Pp5gzY9VfY3/yAn2lpajuZ7oa9zV5yOgXSowNcST0taG5mLk=",
        "T00000LafQ3M4EqfUq3S4v6v9ntSFDCAWgFmBGow": "BOAvNtutyV8BlzRQkACE+rlBW8tgyKcEXFKrHQNEarXYEugu2uDaWOhuiKRuvsslw2gGDdKw72vfx1McmemaYLY=",
        "T00000LT8x79D9BaYjWJHWmjUEYFLF2EeLqre3Qf": "BPOkgoXC/NO0enIKSuAsKQa2i/g71BiVKoMCxwYI2ymUtEzPMeGP3C1erifgKfqlc+s1x6Hncx+/ipC8G+2xGno=",
        "T00000Lc9Y23656TBpH8UdbBdydpy4cJgi6ByZLc": "BKRAfteE8Z2DiVouu5RHdkFoug4uDmMIWTiDsrnp/uQyxBfdLWokGMIwCEFl3NZaq03waHQXKmgIs7V09lLnSUU=",
        "T00000LW9FUWkk7iwMwHrSpsYPgHfxQY5acNY8cX": "BMwQfxcO03SIdmfX3I0YSq9wGBuoGzPtL9c4v0G8B+xPKUxo9hbqvvZjPZ5NYoFgG7eHV5kzzKp6MPjMVDc0p0I=",
        "T00000LKXokSTcTDXahHFa4rgTkcZrXdkRTgddx2": "BH+gZtxEENjvyj8rFEqB6OH4y+HwArbiheUPtSg/YLf+xcenduD9/HM1S0qBER6MdNQ0sb6JOHQWnUDpOntisYE=",
        "T00000LcVpdFoazxyWH2NaKNCzVHojqmWMYXT4Fn": "BA8X6Hv/wzd4weh1hbj9GDCloW5yRV14bBFMTBXg4jy/Z/J18LbM3JpU9Dva1QCyoTJ3DA+YzZC5eeOkrRf+x0s=",
        "T00000LUnFtR4TfQoVUVsijWaaKW4oN7rjdhdivn": "BFArXDZ8JWPr7OYI7ImoI1sfL6q88GscVem8CGUukxLrROzD4B8PA+q49HFXLG1DBHBBq1CGLcJ2j5uTbRt3ujc=",
        "T00000LV82d5eTfPxSrvNGXKsKtYTt2GWGzjpvFX": "BGbeOf1VoQD/RlwiZRWoAqho4pCPy0sClWNrYfXWf8ZBXZQjGihiFvKRI873wt0rAsYU7qqLtKrMG5Vz+fcugbE=",
        "T00000LRKp9wmQCdkr9N4d26EXprPvVUvJHE83XN": "BOa8H3HiRV0OyRGPHec8ljgx7f+7WGO7YQf0/+E37jvf8vc1OY0BV8ydlSl17cXlYUwT8L2uD+THtBASfCblt6s=",
        "T00000Lg7cpUcuj3j6wN5LTVJhNQSUgBNP7YewR3": "BAEhSdmg0TqSL8YY8+gYVdw9ZJIhykLZV3eb7H1kVhJGaFU+j51vLIm/zOFQN5FSBWj2Wy3LKGJIqBNwG+xlr9U=",
        "T00000Lgtmefpdheb3bHc28n42DHA2mibFmadmnT": "BH/BrmXYmedu/CDtkDRyapOvZmwl/jnNgmvlEqyhCJbzuyKbm4VP7RbhFh5Y8zFhcIfHPwXo8sNhYE2GDdiVeaY="
    },
    "timestamp": 1599555555
}
)T";


const std::string & get_genesis_info() {

#if defined(XBUILD_CONSORTIUM)
    #include "xloader/xgenesis_info_consortium.h"
    #if defined(XBUILD_CI)
        return g_ci_consortium_genesis_config;
    #elif defined(XBUILD_DEV)
        return g_dev_consortium_genesis_config;
    #else
        //add other config later
        static_assert(false, "not config for consortium!");
        return ;
    #endif
#else
    #if defined(XBUILD_CI)
        return g_ci_genesis_config;
    #elif defined(XBUILD_GALILEO)
        return g_testnet_genesis_config;
    #elif defined(XBUILD_DEV)
        return g_dev_genesis_config;
    #elif defined(XBUILD_BOUNTY)
        return g_bounty_genesis_config;
    #else
        return g_mainnet_genesis_config;
    #endif
#endif
}

}  // namespace top

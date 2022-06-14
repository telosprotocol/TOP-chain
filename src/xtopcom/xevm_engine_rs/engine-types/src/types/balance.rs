use crate::fmt::Formatter;
use crate::Display;

#[derive(Default, Debug, Clone, Copy, Eq, PartialEq, Ord, PartialOrd)]

pub struct Balance(u128);

impl Display for Balance {
    fn fmt(&self, f: &mut Formatter<'_>) -> crate::fmt::Result {
        self.0.fmt(f)
    }
}

impl Balance {
    /// Constructs a new `Balance` with a given u128 value.
    pub const fn new(amount: u128) -> Balance {
        Self(amount)
    }

    /// Consumes `Balance` and returns the underlying type.
    pub fn as_u128(self) -> u128 {
        self.0
    }
}

pub mod error {
    use crate::{fmt, String};

    #[derive(Eq, Hash, Clone, Debug, PartialEq)]
    pub struct BalanceOverflowError;

    impl AsRef<[u8]> for BalanceOverflowError {
        fn as_ref(&self) -> &[u8] {
            b"ERR_BALANCE_OVERFLOW"
        }
    }

    impl fmt::Display for BalanceOverflowError {
        fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
            let msg = String::from_utf8(self.as_ref().to_vec()).unwrap();
            write!(f, "{}", msg)
        }
    }
}

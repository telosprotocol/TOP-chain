use crate::{fmt, str, str::FromStr, Box, String, Vec};
use borsh::{BorshDeserialize, BorshSerialize};

#[derive(
    Debug, Default, Hash, Clone, BorshSerialize, BorshDeserialize, PartialEq, Eq, PartialOrd, Ord,
)]
pub struct AccountId(Box<str>);

impl AccountId {
    pub fn new(account_id: &str) -> Result<Self, ParseAccountError> {
        Self::validate(account_id)?;
        Ok(Self(account_id.into()))
    }

    pub fn validate(_account_id: &str) -> Result<(), ParseAccountError> {
        // todo add validate
        Ok(())
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.as_ref().as_bytes()
    }
}

impl TryFrom<String> for AccountId {
    type Error = ParseAccountError;

    fn try_from(account_id: String) -> Result<Self, Self::Error> {
        AccountId::new(&account_id)
    }
}

impl TryFrom<&[u8]> for AccountId {
    type Error = ParseAccountError;

    fn try_from(account_id: &[u8]) -> Result<Self, Self::Error> {
        let account_id = str::from_utf8(account_id).map_err(|_| ParseAccountError::Invalid)?;
        AccountId::new(account_id)
    }
}

impl TryFrom<Vec<u8>> for AccountId {
    type Error = ParseAccountError;

    fn try_from(account_id: Vec<u8>) -> Result<Self, Self::Error> {
        AccountId::try_from(&account_id[..])
    }
}

impl FromStr for AccountId {
    type Err = ParseAccountError;

    fn from_str(account_id: &str) -> Result<Self, Self::Err> {
        Self::validate(account_id)?;
        Ok(Self(account_id.into()))
    }
}

impl From<AccountId> for String {
    fn from(account_id: AccountId) -> Self {
        account_id.0.into_string()
    }
}

impl fmt::Display for AccountId {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl From<AccountId> for Box<str> {
    fn from(value: AccountId) -> Box<str> {
        value.0
    }
}

impl From<AccountId> for Vec<u8> {
    fn from(account_id: AccountId) -> Vec<u8> {
        account_id.as_bytes().to_vec()
    }
}

impl<T: ?Sized> AsRef<T> for AccountId
where
    Box<str>: AsRef<T>,
{
    fn as_ref(&self) -> &T {
        self.0.as_ref()
    }
}

#[derive(PartialEq, Eq, Hash, Clone, Debug)]
pub enum ParseAccountError {
    Invalid,
}

impl AsRef<[u8]> for ParseAccountError {
    fn as_ref(&self) -> &[u8] {
        match self {
            ParseAccountError::Invalid => b"ERR_ACCOUNT_ID_INVALID",
        }
    }
}

impl fmt::Display for ParseAccountError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let msg = String::from_utf8(self.as_ref().to_vec()).unwrap();
        write!(f, "{}", msg)
    }
}

// todo (charles) add test

#[derive(Debug)]
pub enum ReadU64Error {
    InvalidU64,
    MissingValue,
}

impl AsRef<[u8]> for ReadU64Error {
    fn as_ref(&self) -> &[u8] {
        match self {
            Self::InvalidU64 => b"ERR_NOT_U64",
            Self::MissingValue => b"ERR_U64_NOT_FOUND",
        }
    }
}

#[derive(Debug)]
pub enum ReadU256Error {
    InvalidU256,
    MissingValue,
}

impl AsRef<[u8]> for ReadU256Error {
    fn as_ref(&self) -> &[u8] {
        match self {
            Self::InvalidU256 => b"ERR_NOT_U256",
            Self::MissingValue => b"ERR_U256_NOT_FOUND",
        }
    }
}

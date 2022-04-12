use primitive_types::U256;

use super::{
    proto_basic::{ProtoAddress, RawU256, WeiU256},
    Address, Wei,
};

impl From<[u8; 32]> for WeiU256 {
    fn from(data: [u8; 32]) -> Self {
        let mut res = WeiU256::new();
        res.set_data(data.into_iter().collect());
        res
    }
}

impl Into<Wei> for WeiU256 {
    fn into(self) -> Wei {
        Wei::new(U256::from_big_endian(&self.get_data()))
    }
}

impl From<[u8; 32]> for RawU256 {
    fn from(data: [u8; 32]) -> Self {
        let mut res = RawU256::new();
        res.set_data(data.into_iter().collect());
        res
    }
}

impl From<ProtoAddress> for Address {
    fn from(proto_address: ProtoAddress) -> Self {
        Address::from_array(
            proto_address
                .get_value()
                .try_into()
                .expect("incorrect length of ProtoAddress data"),
        )
    }
}

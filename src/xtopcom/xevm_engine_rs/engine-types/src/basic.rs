use evm::backend::Log;
use primitive_types::{H160, H256, U256};

use crate::proto_basic::{ProtoAddress, RawU256, ResultLog, WeiU256};
use crate::types::{Address, Wei};

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

impl From<U256> for WeiU256 {
    fn from(data: U256) -> Self {
        let mut res = WeiU256::new();
        let mut result = [0u8; 32];
        data.to_big_endian(&mut result);
        res.set_data(result.to_vec());
        res
    }
}

impl From<&WeiU256> for U256 {
    fn from(data: &WeiU256) -> Self {
        U256::from_big_endian(data.get_data())
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

impl Into<ProtoAddress> for Address {
    fn into(self) -> ProtoAddress {
        let mut address = ProtoAddress::new();
        address.set_value(self.as_bytes().to_vec());
        address
    }
}

impl Into<ProtoAddress> for H160 {
    fn into(self) -> ProtoAddress {
        let mut address = ProtoAddress::new();
        address.set_value(self.as_bytes().to_vec());
        address
    }
}

impl From<&ProtoAddress> for H160 {
    fn from(proto_address: &ProtoAddress) -> Self {
        H160::from_slice(proto_address.get_value())
    }
}

impl From<Log> for ResultLog {
    fn from(log: Log) -> Self {
        let topics: Vec<RawU256> = log.topics.into_iter().map(|topic| topic.0.into()).collect();

        let mut res = ResultLog::new();
        res.set_address(log.address.into());
        res.set_topics(topics.into());
        res.set_data(log.data);

        res
    }
}

impl Into<Log> for ResultLog {
    fn into(self) -> Log {
        Log {
            address: self.get_address().into(),
            topics: self
                .get_topics()
                .into_iter()
                .map(|topic| H256::from_slice(topic.get_data()))
                .collect(),
            data: self.get_data().to_vec(),
        }
    }
}

// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace eosio_evm {
  struct [[eosio::table, eosio::contract("eosio.evm")]] Account {
    uint64_t index;
    eosio::checksum160 address;
    eosio::name account;
    eosio::asset balance;
    uint64_t nonce;
    std::vector<uint8_t> code;

    Account () = default;
    Account (uint256_t _address): address(addressToChecksum160(_address)) {}
    uint64_t primary_key() const { return index; };

    uint64_t get_account_value() const { return account.value; };
    uint256_t get_address() const { return checksum160ToAddress(address); };
    int64_t get_balance() const { return balance.amount; };
    uint64_t get_balance_u64() const { return (uint64_t) balance.amount; };
    uint64_t get_nonce() const { return nonce; };
    std::vector<uint8_t> get_code() const { return code; };
    bool is_empty() const { return nonce == 0 && balance.amount == 0 && code.size() == 0; };

    eosio::checksum256 by_address() const { return pad160(address); };

    void print() const {
      eosio::print("\n---Acc Info Start-----");
      eosio::print("\nAddress ", address);
      eosio::print("\nIndex ", index);
      eosio::print("\nEOS Account " + account.to_string());
      eosio::print("\nBalance ", balance);
      eosio::print("\nNonce ", nonce);
      eosio::print("\n---Acc Info End---\n");
    }

    EOSLIB_SERIALIZE(Account, (index)(address)(account)(balance)(nonce)(code));
  };

  struct [[eosio::table, eosio::contract("eosio.evm")]] AccountState {
    uint64_t index;
    eosio::checksum256 key;
    bigint::checksum256 value;

    uint64_t primary_key() const { return index; };
    eosio::checksum256 by_key() const { return key; };

    EOSLIB_SERIALIZE(AccountState, (index)(key)(value));
  };

  typedef eosio::multi_index<"account"_n, Account,
    eosio::indexed_by<eosio::name("byaddress"), eosio::const_mem_fun<Account, eosio::checksum256, &Account::by_address>>,
    eosio::indexed_by<eosio::name("byaccount"), eosio::const_mem_fun<Account, uint64_t, &Account::get_account_value>>
  > account_table;
  typedef eosio::multi_index<"accountstate"_n, AccountState,
    eosio::indexed_by<eosio::name("bykey"), eosio::const_mem_fun<AccountState, eosio::checksum256, &AccountState::by_key>>
  > account_state_table;
}
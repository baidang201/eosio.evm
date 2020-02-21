// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <eosio.evm/eosio.evm.hpp>

namespace eosio_evm {
  // Only used by CREATE instruction to increment nonce of contract
  void evm::increment_nonce(const Address& address) {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, get_self(), [&](auto& a) {
        a.nonce += 1;
      });
    }
  }

  // Used to push result of top level create and instruction CREATE
  void evm::set_code(const Address& address, const std::vector<uint8_t>& code) {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));
    eosio::print("Searching for ", intx::hex(address));
    eosio::print("Searching for ", toChecksum256(address));
    eosio::print("Found for ", existing_address != accounts_byaddress.end());
    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, get_self(), [&](auto& a) {
        a.code = code;
      });
    }
  }

  // Returns an empty account if not found
  const Account& evm::get_account(const Address& address)
  {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));

    if (existing_address == accounts_byaddress.end())
    {
      static const auto empty_account = Account(address);
      return empty_account;
    }
    else
    {
      return *existing_address;
    }
  }

  // Returns [account, error], error is true if account already exists
  evm::AccountResult evm::create_account(
    const Address& address,
    const int64_t& balance,
    const bool& is_contract // default false
  )
  {
    // Find address
    auto address_160        = addressToChecksum160(address);
    auto address_256        = pad160(address_160);
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address   = accounts_byaddress.find(address_256);

    // ERROR
    if (existing_address != accounts_byaddress.end()) {
      static const auto empty_account = Account(address);
      return { empty_account, true };
    }

    // Initial Nonce is 1 for contracts and 0 for accounts (no code)
    uint64_t nonce = is_contract ? 1 : 0;

    // Create address if it does not exists
    auto new_address = _accounts.emplace(get_self(), [&](auto& a) {
      a.index   = _accounts.available_primary_key();
      a.nonce   = nonce;
      a.address = address_160;
      a.balance = eosio::asset(balance, TOKEN_SYMBOL);
    });

    return { *new_address, false };
  }

  /**
   * Used for self-destruct
   */
  void evm::selfdestruct(const Address& address)
  {
    auto accounts_byaddress = _accounts.get_index<eosio::name("byaddress")>();
    auto existing_address = accounts_byaddress.find(toChecksum256(address));

    if (existing_address != accounts_byaddress.end()) {
      accounts_byaddress.modify(existing_address, eosio::same_payer, [&](auto& a) {
        a.nonce = 0;
        a.balance.amount = 0;
        a.code = std::vector<uint8_t>{};
      });
    }
  }

  // Returns original state
  void evm::storekv(const uint64_t& address_index, const uint256_t& key, const uint256_t& value) {
    // Get scoped state table for account state
    account_state_table accounts_states(get_self(), address_index);
    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
    auto checksum_key          = toChecksum256(key);
    auto account_state         = accounts_states_bykey.find(checksum_key);

    // eosio::print("\n\nStore KV for address ", intx::hex(address),
    //              "\nKey: ", to_string(key, 10),
    //              "\nValue ", to_string(value, 10),
    //              "\nAddress Index: ", to_string(address_index),
    //              "\nFound: ", account_state != accounts_states_byaddress.end(), "\n");

    // Key found
    if (account_state != accounts_states_bykey.end())
    {
      if (value == 0) {
        accounts_states_bykey.erase(account_state);
      } else {
        accounts_states_bykey.modify(account_state, eosio::same_payer, [&](auto& a) {
          a.value = value;
        });
      }
    }
    // Key not found
    else
    {
      accounts_states.emplace(get_self(), [&](auto& a) {
        a.index   = accounts_states.available_primary_key();
        a.key     = checksum_key;
        a.value   = value;
      });
    }
  }

  uint256_t evm::loadkv(const uint64_t& address_index, const uint256_t& key) {
    // Get scoped state table for account
    account_state_table accounts_states(get_self(), address_index);
    auto accounts_states_bykey = accounts_states.get_index<eosio::name("bykey")>();
    auto checksum_key          = toChecksum256(key);
    auto account_state         = accounts_states_bykey.find(checksum_key);

    // eosio::print("\n\nLoad KV for address ", intx::hex(address),
    //              "\nKey: ", key,
    //              "\nAddress Index: ", to_string(address_index),
    //              "\nFound: ", account_state != accounts_states_bykey.end(), "\n");

    // Key found
    if (account_state != accounts_states_bykey.end())
    {
      // eosio::print("\nFound KV Value ", to_string(account_state->value, 10));
      return account_state->value;
    }
    // Key not found
    else {
      return 0;
    }
  }
} // namespace eosio_evm
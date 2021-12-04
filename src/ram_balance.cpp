#include <shomaiiblend.hpp>

/**
 * Inline action fro depositing ram.
*/
[[eosio::on_notify("eosio.token::transfer")]] void shomaiiblend::depositram(name from, name to, asset quantity, string memo) {
    const set<name> ignore = set<name>{
        atomicassets::ATOMICASSETS_ACCOUNT,
        // EOSIO system accounts
        name("eosio.stake"),
        name("eosio.names"),
        name("eosio.ram"),
        name("eosio.rex"),
        name("eosio")};

    if (to != get_self() || ignore.find(from) != ignore.end()) {
        return;
    }

    check(get_first_receiver() == name("eosio.token") && quantity.symbol == symbol("WAX", 8),
          "Must transfer core token when depositing RAM");

    auto collection = name(memo);

    // confirm and verify collection
    check(memo.size() <= 12, "Invalid collection name!");
    atomicassets::collections.require_find(collection.value, "Collection does not exist!");

    action(
        permission_level{get_self(), name("active")},
        get_self(),
        name("buyramproxy"),
        std::make_tuple(
            collection,
            quantity))
        .send();
}

/**
 * Withdraw ram to user.
*/
ACTION shomaiiblend::withdrawram(name author, name collection, name recipient, int64_t bytes) {
    require_auth(author);

    check(isAuthorized(collection, author), "User is not authorized in collection.");
    check(is_account(recipient), "Recipient account does not exist.");

    decrease_ram_balance(collection, bytes);

    asset payout = ram::get_sell_ram_quantity(bytes);

    action(
        permission_level{get_self(), name("active")},
        name("eosio"),
        name("sellram"),
        make_tuple(
            get_self(),
            bytes))
        .send();

    action(
        permission_level{get_self(), name("active")},
        name("eosio.token"),
        name("transfer"),
        make_tuple(
            get_self(),
            recipient,
            payout,
            string("Sold RAM")))
        .send();
}

/* https://github.com/pinknetworkx/atomicpacks-contract/blob/master/src/ram_handling.cpp#L49
* Action that can only be called by the contract itself.
* 
* Having this in an extra action rather than directly in the on transfer notify is needed in order to
* prevent a reentrency attack that would otherwise open up due to the execution order of notifications
* and inline actions
*/
ACTION shomaiiblend::buyramproxy(name collection, asset quantity) {
    require_auth(get_self());

    increase_ram_balance(collection, ram::get_purchase_ram_bytes(quantity));

    action(
        permission_level{get_self(), name("active")},
        name("eosio"),
        name("buyram"),
        std::make_tuple(
            get_self(),
            get_self(),
            quantity))
        .send();
}

/**
 * Internal function to decrease the ram balance.
*/
void shomaiiblend::decrease_ram_balance(name collection, int64_t bytes) {
    check(bytes > 0, "Bytes should be creater than zero.");

    auto itr = rambalances.find(collection.value);
    check(itr != rambalances.end() && itr->bytes >= bytes, "Collection does not have enough ram balance.");

    rambalances.modify(itr, _self, [&](rambalance_s &row) {
        row.bytes -= bytes;
    });
}

/**
 * Internal function to increase the ram balance.
*/
void shomaiiblend::increase_ram_balance(name collection, int64_t bytes) {
    check(bytes > 0, "Bytes should be creater than zero.");

    auto itr = rambalances.find(collection.value);

    if (itr == rambalances.end()) {
        check(bytes >= 144, "Ram balance should be greater than 144 for the table entry.");  // this is for the claimjob entry (TODO: find a good alt)
        rambalances.emplace(_self, [&](rambalance_s &row) {
            row.collection = collection;
            row.bytes = bytes - 144;
        });
        return;
    }

    rambalances.modify(itr, _self, [&](rambalance_s &row) {
        row.bytes += bytes;
    });
}
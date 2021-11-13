#include <shomaiiblend.hpp>

#include "call_blend.cpp"
#include "claims.cpp"
#include "configs.cpp"
#include "internals.cpp"
#include "make_blend.cpp"
#include "remove_blend.cpp"

/**
 * Initialize main config singleton db.
*/
ACTION shomaiiblend::init() {
    require_auth(get_self());

    config.get_or_create(_self, config_s{});
}

/**
 * Initialize system config singleton db.
*/
ACTION shomaiiblend::initsys() {
    require_auth(get_self());

    sysconfig.get_or_create(_self, sysconfig_s{});
}

/**
 * Add a collection to the service whitelists.
*/
ACTION shomaiiblend::sysaddwhite(name collection) {
    require_auth(get_self());

    // require check if collection exists or not
    atomicassets::collections.require_find(collection.value, "Collection does not exist!");

    auto _sysconfig = sysconfig.get();

    if (find(_sysconfig.whitelists.begin(), _sysconfig.whitelists.end(), collection) == _sysconfig.whitelists.end()) {
        // if collection does not exist, push it to the vector
        _sysconfig.whitelists.push_back(collection);

        // set new whitelist collections
        sysconfig.set(_sysconfig, get_self());
    }
}

/**
 * Add a collection to the service blacklist.
*/
ACTION shomaiiblend::sysaddblack(name collection) {
    require_auth(get_self());

    // require check if collection exists or not
    atomicassets::collections.require_find(collection.value, "Collection does not exist!");

    auto _sysconfig = sysconfig.get();

    if (find(_sysconfig.blacklists.begin(), _sysconfig.blacklists.end(), collection) == _sysconfig.blacklists.end()) {
        // if collection does not exist, push it to the vector
        _sysconfig.blacklists.push_back(collection);

        // set new whitelist collections
        sysconfig.set(_sysconfig, get_self());
    }
}

/**
 * Remove NFTs from refunds. (for emergency / needed purposes)
*/
ACTION shomaiiblend::clearrefunds(name scope) {
    require_auth(get_self());

    // remove all recorsd
    auto reftable = get_nftrefunds(scope);
    auto itr = reftable.begin();

    while (itr != reftable.end()) {
        reftable.erase(itr);
    }
}

/**
 * Log NFT transfers for refund.
*/
[[eosio::on_notify("atomicassets::transfer")]] void shomaiiblend::savetransfer(name from, name to, vector<uint64_t> asset_ids, string memo) {
    // ignore sent nfts by this contract
    if (from == get_self()) {
        return;
    }

    // check collection name
    check(memo.size() == 12, "Collection name in memo is too long!");

    name col = name(memo);
    nftrefund_t refundtable = get_nftrefunds(from);

    // save all nfts
    for (auto i : asset_ids) {
        refundtable.emplace(get_self(), [&](nftrefund_s &row) {
            row.assetid = i;
            row.collection = col;
            row.from = from;
        });
    }
}

/**
 * Refund action for NFTs transferred but were not processed in blends.
 * 
*/
ACTION shomaiiblend::refundnfts(name user, name scope, vector<uint64_t> assetids) {
    require_auth(user);
    blockContract(user);

    auto refundtable = get_nftrefunds(user);

    // check all assets and confirm
    for (auto i : assetids) {
        auto itr = refundtable.require_find(i, "Asset does not exist in collection for refund!");

        check(itr->collection == scope, "Asset's collection is not similar to scope!");  // useless check
        check(itr->from == user, "Asset is not from user!");
    }

    // transfer NFTs
    action(
        permission_level{get_self(), name("active")},
        ATOMICASSETS,
        name("transfer"),
        make_tuple(get_self(), user, assetids, string("nft refund from shomai blends")))
        .send();

    // remove from refunds
    removeRefundNFTs(user, scope, assetids);
}
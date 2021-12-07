/**
 * BLENDING Process
 *  -> Calling of smart contract is chained.
 *      - Transfer first the NFT to the smart contract.
 *      - Another transaction is needed to process the blending.
*/
#pragma once

#include <atomicassets.hpp>
#include <custom-types.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <ram-interface.hpp>
#include <wax-orng.hpp>

using namespace std;
using namespace eosio;

#define ATOMICASSETS name("atomicassets")

const uint32_t TOTALODDS = 100;

CONTRACT shomaiiblend : public contract {
   public:
    using contract::contract;

    /* Start Blend Actions */
    ACTION makeblsimple(name author, name collection, uint32_t target, vector<uint32_t> ingredients);
    ACTION makeswsimple(name author, name collection, uint32_t target, uint32_t ingredient);
    ACTION makeblmulti(name author, name collection, uint32_t target, vector<MultiBlendIngredient> ingredients, string title);
    ACTION makeblslot(name author, name collection, vector<MultiTarget> targets, vector<SlotBlendIngredient> ingredients, string title);

    ACTION remblsimple(name user, name scope, uint64_t blenderid);
    ACTION remswsimple(name user, name scope, uint64_t blenderid);
    ACTION remblslot(name user, name scope, uint64_t blenderid);

    ACTION callblsimple(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids);
    ACTION callswsimple(uint64_t blenderid, name blender, name scope, uint64_t asset);
    ACTION callblslot(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids, uint64_t claim_id);

    ACTION claimblslot(uint64_t claim_id, name blender, name scope);

    ACTION removeconfig(name author, uint64_t blenderid, name scope);
    ACTION setwhitelist(name author, uint64_t blenderid, name scope, vector<name> names_list);
    ACTION setonwhlist(name author, uint64_t blenderid, name scope, bool on_whitelist);
    ACTION setdates(name author, uint64_t blenderid, name scope, int32_t startdate, int32_t enddate);
    ACTION setmax(name author, uint64_t blenderid, name scope, int32_t maxuse, int32_t maxuseruse, int32_t maxusercooldown);
    /* End Blend Actions */

    /* Start ORNG Actions */
    ACTION receiverand(uint64_t assoc_id, checksum256 random_value);
    /* End ORNG Actions */

    /*  Start System actions */
    ACTION init();
    ACTION initsys();
    ACTION sysaddwhite(name collection);
    ACTION sysaddblack(name collection);
    ACTION clearrefunds(name scope);
    /*  End System actions */

    /*  Start Ram actions */
    ACTION withdrawram(name author, name collection, name recipient, int64_t bytes);
    /*  End Ram actions */

    /* Start Util actions */
    ACTION refundnfts(name user, name scope, vector<uint64_t> assetids);
    ACTION buyramproxy(name collection, asset quantity);
    /* End Util actions */

    /* Start Payable Actions */
    [[eosio::on_notify("eosio.token::transfer")]] void depositram(name from, name to, asset quantity, string memo);
    [[eosio::on_notify("atomicassets::transfer")]] void savetransfer(name from, name to, vector<uint64_t> asset_ids, string memo);
    /* End Payable Actions */

   private:
    TABLE rambalance_s {
        name collection;
        uint64_t bytes;

        uint64_t primary_key() const { return collection.value; };
    };

    /*
   System configuration singleton table.
   */
    TABLE sysconfig_s {
        vector<name> whitelists;
        vector<name> blacklists;
    };

    /*
    Refund NFT Table.
     - This is where NFT transactions are logged for refund if there are failed transactions.
  */
    TABLE nftrefund_s {
        uint64_t assetid;
        name from;
        name collection;

        uint64_t primary_key() const { return assetid; };
    };

    /*
  Simple Blend (only from one collection)
*/
    TABLE simpleblend_s {
        uint64_t blenderid;
        name author;  // author

        name collection;
        uint32_t target;
        vector<uint32_t> ingredients;  // ingredients should only be from collection

        uint64_t primary_key() const { return blenderid; };
        // uint64_t by_collection() const { return collection.value; };
    };

    /*
  Multi Blend (cross-collection, )
  */
    TABLE multiblend_s {
        uint64_t blenderid;
        name author;

        name collection;
        uint32_t target;
        vector<MultiBlendIngredient> ingredients;

        string title;

        uint64_t primary_key() const { return blenderid; };
    };

    /*
  Slot Blend (config-based)
 */
    TABLE slotblend_s {
        uint64_t blenderid;
        name author;

        name collection;
        vector<SlotBlendIngredient> ingredients;

        string title;

        uint64_t primary_key() const { return blenderid; };
    };

    /*
  Simple Swap (swap assets, single collection)
*/
    TABLE simpleswap_s {
        uint64_t blenderid;
        name author;

        name collection;  // fromtemp and totemp can only be from similar collection
        uint32_t ingredient;
        uint32_t target;

        uint64_t primary_key() const { return blenderid; };
        // uint64_t by_collection() const { return collection.value; };
    };

    /*
	Multi Target pool.
*/
    TABLE multitarget_s {
        uint64_t blenderid;

        name collection;
        vector<MultiTarget> targets;

        uint64_t primary_key() const { return blenderid; };
    };

    /*
	Unclaimed NFTs from blends.
	*/
    TABLE claimassets_s {
        uint64_t claim_id;

        uint64_t blenderid;
        name blender;

        int32_t templateid;
        vector<uint64_t> assets;

        uint64_t primary_key() const { return claim_id; };
    };

    TABLE claimjob_s {
        uint64_t claim_id;

        uint64_t blenderid;
        name blender;
        name scope;               // collection name
        vector<uint64_t> assets;  // ingredients

        uint64_t primary_key() const { return claim_id; };
    };

    /*
    BlendConfigs
  */
    TABLE blendconfig_s {
        uint64_t blenderid;

        int32_t maxuse = -1;           // -1 = (global use) infinite use, 0 = disabled
        int32_t maxuseruse = -1;       // -1 = (user use) infinite use
        int32_t maxusercooldown = -1;  // -1 = no cooldown

        int32_t startdate = -1;  // -1, start as soon
        int32_t enddate = -1;    // -1, does not end

        vector<name> whitelists = {};    // for whitelisting
        bool enable_whitelists = false;  // on whitelists, even if this is changed, the `whitelists` field will not be changed nor modified

        uint64_t primary_key() const { return blenderid; };
    };

    /**
     * Blend uses management. This table stores the blend uses per user.
    */
    TABLE blendconfiguses_s {
        uint64_t blenderid;

        name blender;
        int32_t last_used;
        int32_t uses;

        uint64_t primary_key() const { return blenderid; };
    };

    /**
     * Blend stats counter on usage.
    */
    TABLE blendstats_s {
        uint64_t blenderid;
        uint32_t total_uses;

        uint64_t primary_key() const { return blenderid; };
    };

    TABLE config_s {
        uint64_t blendercounter = 100000;
        uint64_t claimcounter = 100000;
    };

    typedef multi_index<"rambalances"_n, rambalance_s> rambalance_t;

    typedef singleton<"configs"_n, config_s> config_t;
    typedef multi_index<"configs"_n, config_s> config_t_for_abi;
    typedef singleton<"sysconfigs"_n, sysconfig_s> sysconfig_t;
    typedef multi_index<"sysconfigs"_n, sysconfig_s> sysconfig_t_for_abi;

    typedef multi_index<"simblenders"_n, simpleblend_s> simblender_t;
    typedef multi_index<"multblenders"_n, multiblend_s> multiblend_t;
    typedef multi_index<"slotblenders"_n, slotblend_s> slotblend_t;
    typedef multi_index<"simswaps"_n, simpleswap_s> simswap_t;

    typedef multi_index<"blendconfig"_n, blendconfig_s> blendconfig_t;
    typedef multi_index<"blendcfuses"_n, blendconfiguses_s> blendconfiguses_t;
    typedef multi_index<"blendstats"_n, blendstats_s> blendstats_t;
    typedef multi_index<"nftrefunds"_n, nftrefund_s> nftrefund_t;

    typedef multi_index<"targetpools"_n, multitarget_s> multitargetpool_t;
    typedef multi_index<"claimassets"_n, claimassets_s> claimassets_t;
    typedef multi_index<"claimjobs"_n, claimjob_s> claimjob_t;

    //  indexed_by<"collection"_n, const_mem_fun<simpleswap_s, uint64_t, &simpleswap_s::by_collection>>

    /* Initialize tables */
    config_t config = config_t(_self, _self.value);
    sysconfig_t sysconfig = sysconfig_t(_self, _self.value);
    claimjob_t claimjobs = claimjob_t(_self, _self.value);
    rambalance_t rambalances = rambalance_t(_self, _self.value);

    /* Internal get tables by scope. */

    // get simple blends of collecton
    simblender_t get_simpleblends(name collection) {
        return simblender_t(_self, collection.value);
    }

    // get simple swaps of collection
    simswap_t get_simpleswaps(name collection) {
        return simswap_t(_self, collection.value);
    }

    multiblend_t get_multiblends(name collection) {
        return multiblend_t(_self, collection.value);
    }

    // get slot blends of collection
    slotblend_t get_slotblends(name collection) {
        return slotblend_t(_self, collection.value);
    }

    // get blendconfigs of collection
    blendconfig_t get_blendconfigs(name collection) {
        return blendconfig_t(_self, collection.value);
    }

    // get blenduses of the user
    blendconfiguses_t get_userblends(name user) {
        return blendconfiguses_t(_self, user.value);
    }

    // get blend stats
    blendstats_t get_blendstats(name collection) {
        return blendstats_t(_self, collection.value);
    }

    // get nft refunds of collection (use user as scope)
    nftrefund_t get_nftrefunds(name user) {
        return nftrefund_t(_self, user.value);
    }

    // get multitarget pool
    multitargetpool_t get_blendertargets(name collection) {
        return multitargetpool_t(_self, collection.value);
    }

    // get claim assets
    claimassets_t get_claimassets(name collection) {
        return claimassets_t(_self, collection.value);
    }

    // get blender id
    uint64_t get_blenderid() {
        // get burner counter
        config_s current_config = config.get();
        uint64_t blenderid = current_config.blendercounter++;
        config.set(current_config, get_self());

        return blenderid;
    }

    atomicassets::collections_t::const_iterator get_collection(name author, name collection);
    atomicassets::templates_t::const_iterator get_target_template(name scope, uint64_t target_template);
    atomicassets::assets_t::const_iterator validateasset(uint64_t asset, name owner);

    // ======== util functions
    void validate_template_ingredient(atomicassets::templates_t & templates, uint64_t assetid);
    void validate_multitarget(name collection, vector<MultiTarget> targets);
    void validate_caller(name user, name collection);

    void check_config(uint64_t blenderid, name blender, name scope);
    void remove_blend_config(uint64_t blenderid, name author, name scope);
    void remove_blend_stats(uint64_t blenderid, name author, name scope);
    void increment_blend_use(uint64_t blenderid, name blender, name scope);
    blendconfig_t::const_iterator set_config_check(name author, uint64_t blenderid, name scope);

    // ram actions
    void decrease_ram_balance(name collection, int64_t bytes);
    void increase_ram_balance(name collectiom, int64_t bytes);
    void check_ram_balance(name collection);

    // ======== sys configs
    bool isWhitelisted(name collection);
    bool isBlacklisted(name collection);

    void checkfromrefund(uint64_t assetid, name owner);
    void removeRefundNFTs(name from, name collection, vector<uint64_t> assetids);

    bool isAuthorized(name collection, name user);

    void mintasset(name collection, name schema, uint64_t templateid, name to);
    void burnassets(vector<uint64_t> assets);
    void transferassets(vector<uint64_t> assets, name to);

    /*
  Block the smart contract from calling own functions.
  */
    void blockContract(name caller) {
        check(caller != _self, "The smart contract should not call any of it's own functions!");
    }

    int32_t now() {
        return current_time_point().sec_since_epoch();
    }
};

/**
 * BLENDING Process
 *  -> Calling of smart contract is chained.
 *      - Transfer first the NFT to the smart contract.
 *      - Another transaction is needed to process the blending.
*/
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>

#include <atomicassets.hpp>
#include <atomicdata.hpp>
#include <custom-types.hpp>
#include <wax-orng.hpp>

using namespace std;
using namespace eosio;

#define ATOMICASSETS name("atomicassets")

const uint32_t TOTALODDS = 100;

CONTRACT shomaiiblend : public contract
{
public:
	using contract::contract;

	/* Start Blend Actions */
	ACTION makeblsimple(name author, name collection, uint64_t target, vector<uint64_t> ingredients);
	ACTION makeswsimple(name author, name collection, uint64_t target, uint64_t ingredient);
	ACTION makeblmulti(name author);
	ACTION makeblslot(name author, name collection, vector<MultiTarget> targets, vector<SlotBlendIngredient> ingredients);

	ACTION remblsimple(name user, name scope, uint64_t blenderid);
	ACTION remswsimple(name user, name scope, uint64_t blenderid);

	ACTION callblsimple(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids);
	ACTION callswsimple(uint64_t blenderid, name blender, name scope, uint64_t asset);
	ACTION callblslot(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids);

	ACTION claimblslot(uint64_t claim_id, name blender, name scope);

	ACTION setblsimconf(name author, uint64_t blenderid, name scope, BlendConfig config);
	/* End Blend Actions */

	/* Start ORNG Actions */
	ACTION receiverand(uint64_t claim_id, checksum256 random_value);
	/* End ORNG Actions */

	/*  Start System actions */
	ACTION init();
	ACTION clearrefunds(name scope);
	/*  End System actions */

	/* Start Util actions */
	ACTION refundnfts(name user, name scope, vector<uint64_t> assetids);
	/* End Util actions */

	/* Start Payable Actions */
	[[eosio::on_notify("atomicassets::transfer")]] void savetransfer(name from, name to, vector<uint64_t> asset_ids, string memo);
	/* End Payable Actions */

private:
	/*
    Refund NFT Table.
     - This is where NFT transactions are logged for refund if there are failed transactions.
  */
	TABLE nftrefund_s
	{
		uint64_t assetid;
		name from;
		name collection;

		uint64_t primary_key() const { return assetid; };
	};

	/*
  Simple Blend (only from one collection)
*/
	TABLE simpleblend_s
	{
		uint64_t blenderid;
		name author; // author

		name collection;
		uint64_t target;
		vector<uint64_t> ingredients; // ingredients should only be from collection

		uint64_t primary_key() const { return blenderid; };
		// uint64_t by_collection() const { return collection.value; };
	};

	/*
  Multi Blend (cross-collection, )
  */

	/*
  Slot Blend (config-based)
 */
	TABLE slotblend_s
	{
		uint64_t blenderid;
		name author;

		name collection;
		vector<SlotBlendIngredient> ingredients;

		uint64_t primary_key() const { return blenderid; };
	};

	/*
  Simple Swap (swap assets, single collection)
*/
	TABLE simpleswap_s
	{
		uint64_t blenderid;
		name author;

		name collection; // fromtemp and totemp can only be from similar collection
		uint64_t ingredient;
		uint64_t target;

		uint64_t primary_key() const { return blenderid; };
		// uint64_t by_collection() const { return collection.value; };
	};

	/*
	Multi Target pool.
*/
	TABLE multitarget_s
	{
		uint64_t blenderid;

		name collection;
		vector<MultiTarget> targets;

		uint64_t primary_key() const { return blenderid; };
	};

	/*
	Unclaimed NFTs from blends.
	*/
	TABLE claimassets_s
	{
		uint64_t claim_id;

		uint64_t blenderid;
		name blender;

		int32_t templateid;
		vector<uint64_t> assets;

		uint64_t primary_key() const { return claim_id; };
	};

	TABLE claimjob_s
	{
		uint64_t claim_id;

		uint64_t blenderid;
		name blender;
		name scope;				 // collection name
		vector<uint64_t> assets; // ingredients

		uint64_t primary_key() const { return claim_id; };
	};

	/*
    BlendConfigs
  */
	TABLE blendconfig_s
	{
		uint64_t blenderid;
		BlendConfig config;

		uint64_t primary_key() const { return blenderid; };
	};

	TABLE config_s
	{
		uint64_t blendercounter = 100000;
		uint64_t claimcounter = 100000;
	};

	typedef singleton<"configs"_n, config_s> config_t;
	typedef multi_index<"configs"_n, config_s> config_t_for_abi;

	typedef multi_index<"simblender"_n, simpleblend_s> simblender_t;
	typedef multi_index<"slotblender"_n, slotblend_s> slotblend_t;
	typedef multi_index<"simswap"_n, simpleswap_s> simswap_t;

	typedef multi_index<"blendconfig"_n, blendconfig_s> blendconfig_t;
	typedef multi_index<"refundnft"_n, nftrefund_s> nftrefund_t;

	typedef multi_index<"mtargetpool"_n, multitarget_s> multitargetpool_t;
	typedef multi_index<"claimassets"_n, claimassets_s> claimassets_t;
	typedef multi_index<"claimjobs"_n, claimjob_s> claimjob_t;

	//  indexed_by<"collection"_n, const_mem_fun<simpleswap_s, uint64_t, &simpleswap_s::by_collection>>

	/* Initialize tables */
	config_t config = config_t(_self, _self.value);
	claimjob_t claimjobs = claimjob_t(_self, _self.value);

	/* Internal get tables by scope. */

	// get simple blends of collecton
	simblender_t get_simpleblends(name collection)
	{
		return simblender_t(_self, collection.value);
	}

	// get simple swaps of collection
	simswap_t get_simpleswaps(name collection)
	{
		return simswap_t(_self, collection.value);
	}

	// get slot blends of collection
	slotblend_t get_slotblends(name collection)
	{
		return slotblend_t(_self, collection.value);
	}

	// get blendconfigs of collection
	blendconfig_t get_blendconfigs(name collection)
	{
		return blendconfig_t(_self, collection.value);
	}

	// get nft refunds of collection
	nftrefund_t get_nftrefunds(name collection)
	{
		return nftrefund_t(_self, collection.value);
	}

	// get multitarget pool
	multitargetpool_t get_blendertargets(name collection)
	{
		return multitargetpool_t(_self, collection.value);
	}

	// get claim assets
	claimassets_t get_claimassets(name collection)
	{
		return claimassets_t(_self, collection.value);
	}

	// ======== util functions
	void validate_template_ingredient(atomicassets::templates_t & templates, uint64_t assetid);
	void validate_multitarget(name collection, vector<MultiTarget> targets);

	/*
    Remove NFTs from refund after a successfull action.
  */
	void removeRefundNFTs(name from, name collection, vector<uint64_t> assetids)
	{
		auto refundtable = get_nftrefunds(collection);

		for (auto i : assetids)
		{
			auto itr = refundtable.find(i);

			// some checking in here for sure
			check(itr->from == from, "The asset does not come from you!");
			check(itr->collection == collection, "The asset was not sent to the collection's blend."); // kind of useless check in here

			// remove it
			refundtable.erase(itr);
		}
	}

	/*
      Check if user is authorized to mint NFTs
   */
	bool isAuthorized(name collection, name user)
	{
		auto itr = atomicassets::collections.require_find(collection.value, "No collection with this name exists!");
		bool authorized = false;
		vector<name> accs = itr->authorized_accounts;
		for (auto it = accs.begin(); it != accs.end() && !authorized; it++)
		{
			if (user == name(*it))
			{
				authorized = true;
			}
		}
		return authorized;
	}

	/*
      Call AtomicAssets contract to mint a new NFT
   */
	void mintasset(name collection, name schema, uint64_t templateid, name to)
	{
		vector<asset> back_tokens;
		atomicassets::ATTRIBUTE_MAP nodata = {};

		int32_t _template = int32_t(templateid);

		// call contract
		action(
			permission_level{get_self(), name("active")},
			ATOMICASSETS,
			name("mintasset"),
			make_tuple(get_self(), collection, schema, _template, to, nodata, nodata, back_tokens))
			.send();
	}

	/*
      Call AtomicAssets contract to burn NFTs
      (multiple assets)
   */
	void burnassets(vector<uint64_t> assets)
	{
		for (auto it : assets)
		{
			action(permission_level{get_self(), name("active")},
				   ATOMICASSETS,
				   name("burnasset"),
				   make_tuple(get_self(), it))
				.send();
		}
	}

	/*
  Block the smart contract from calling own functions.
  */
	void blockContract(name caller)
	{
		check(caller != _self, "The smart contract should not call any of it's own functions!");
	}
};

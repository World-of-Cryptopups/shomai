#include <shomaiiblend.hpp>

/**
 * Internal function for checking and validating a template ingredient.
*/
void shomaiiblend::validate_template_ingredient(atomicassets::templates_t &templates, uint64_t assetid) {
    auto itrIngredient = templates.find(assetid);

    check(itrIngredient != templates.end(), "Template ingredient does not exist in collection!");
    check(itrIngredient->transferable, "Template ingredient is not transferable!");
    check(itrIngredient->burnable, "Template ingredient is not burnable!");
}

/**
 * Internal function to validate the target outcomes.
*/
void shomaiiblend::validate_multitarget(name collection, vector<MultiTarget> targets) {
    uint32_t total_counted_odds = 0;
    uint32_t lastodd = UINT32_MAX;

    auto templates = atomicassets::get_templates(collection);

    for (MultiTarget i : targets) {
        // check first if target template exists or not
        auto itrTemplate = templates.require_find(uint64_t(i.templateid), ("Target template does not exist in collection: " + to_string(i.templateid)).c_str());

        check(i.odds > 0, "Each target outcome must have positive odds.");
        check(i.odds <= lastodd, "The target outcome must be sorted in descending order based on their odds.");
        lastodd = i.odds;

        total_counted_odds += i.odds;
        check(total_counted_odds >= i.odds, "Overflow: Total odds can't be more than 2^32 - 1.");

        // this is a multi target blend, so it should not have a max supply.
        check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Can only use templates without a max supply.");
    }

    // check only if there are more than 1 targets
    if (targets.size() > 1) {
        check(total_counted_odds == TOTALODDS, "Totals odds of target outcomes does not equal the provided total odds.");
    }
}

/**
 * This checks if the caller's collection is blacklisted or is whitelisted.
*/
void shomaiiblend::validate_caller(name user, name collection) {
    require_auth(user);
    blockContract(user);

    check(isWhitelisted(collection), "Collection is not whitelisted to use this service! Please ask the owner to whitelist you.");
    check(!isBlacklisted(collection), "Collection is blacklisted from service!");
}

/**
 * Checks if the collection is whitelisted.
*/
bool shomaiiblend::isWhitelisted(name collection) {
    auto _sysconfig = sysconfig.get();

    return find(_sysconfig.whitelists.begin(), _sysconfig.whitelists.end(), collection) != _sysconfig.whitelists.end();
}

/**
 * Checks if the collection is blacklisted.
*/
bool shomaiiblend::isBlacklisted(name collection) {
    auto _sysconfig = sysconfig.get();

    return find(_sysconfig.blacklists.begin(), _sysconfig.blacklists.end(), collection) != _sysconfig.blacklists.end();
}

/**
 * Checks if the asset is transferred to the smart contract and if it exists in the refund table.
 * 
 * Returns the asset iterator.
*/
atomicassets::assets_t::const_iterator shomaiiblend::validateasset(uint64_t asset, name owner) {
    auto itrAssets = atomicassets::get_assets(get_self());
    auto itr = itrAssets.require_find(asset, "The asset is not transferred to the smart contract for blending!");

    checkfromrefund(asset, owner);

    return itr;
}

// get the collection from the atomicassets contract
atomicassets::collections_t::const_iterator shomaiiblend::get_collection(name author, name collection) {
    // validate target collection
    auto itrCol = atomicassets::collections.require_find(collection.value, "This collection does not exist!");

    // validate author
    check(isAuthorized(collection, author), "You are not authorized in this collection!");

    // validate contract is authorized by collection
    check(isAuthorized(collection, author), "Contract is not authorized in the collection!");

    return itrCol;
}

// get the target template iterator
atomicassets::templates_t::const_iterator shomaiiblend::get_target_template(name scope, uint64_t target_template) {
    auto templates = atomicassets::get_templates(scope);
    auto itrTemplate = templates.require_find(target_template, "Target template not found from collection!");

    // check collection mint limit and supply
    check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Blender cannot mint more assets for the target template id!");

    return itrTemplate;
}

/*
      Call AtomicAssets contract to mint a new NFT
   */
void shomaiiblend::mintasset(name collection, name schema, uint64_t templateid, name to) {
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
   */
void shomaiiblend::burnassets(vector<uint64_t> assets) {
    for (auto it : assets) {
        action(permission_level{get_self(), name("active")},
               ATOMICASSETS,
               name("burnasset"),
               make_tuple(get_self(), it))
            .send();
    }
}

/*
    Call AtomicAssets contract to transfer assets
*/
void shomaiiblend::transferassets(vector<uint64_t> assets, name to) {
    action(
        permission_level{get_self(), name("active")},
        ATOMICASSETS,
        name("transfer"),
        make_tuple(get_self(), to, assets, string("transfer from contract")))
        .send();
}

/*
      Check if user is authorized to mint NFTs
   */
bool shomaiiblend::isAuthorized(name collection, name user) {
    auto itr = atomicassets::collections.require_find(collection.value, "No collection with this name exists!");
    bool authorized = false;
    vector<name> accs = itr->authorized_accounts;
    for (auto it = accs.begin(); it != accs.end() && !authorized; it++) {
        if (user == name(*it)) {
            authorized = true;
        }
    }
    return authorized;
}

/**
	 * Checks and confirms if the asset is in the nftrefunds table within the scope of the owner.
	*/
void shomaiiblend::checkfromrefund(uint64_t assetid, name owner) {
    auto refunds = get_nftrefunds(owner);
    refunds.require_find(assetid, "The asset does not exist or is not transferred by the user to the smart contract!");
}

/*
    Remove NFTs from refund after a successfull action.
  */
void shomaiiblend::removeRefundNFTs(name from, name collection, vector<uint64_t> assetids) {
    auto refundtable = get_nftrefunds(from);

    for (auto i : assetids) {
        auto itr = refundtable.find(i);

        // some checking in here for sure
        check(itr->from == from, "The asset does not come from you!");
        check(itr->collection == collection, "The asset was not sent to the collection's blend.");  // kind of useless check in here

        // remove it
        refundtable.erase(itr);
    }
}

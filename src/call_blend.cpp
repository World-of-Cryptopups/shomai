#include <shomaiiblend.hpp>

/**
 * Call Simple Blend.
*/
ACTION shomaiiblend::callblsimple(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids)
{
    require_auth(blender);
    blockContract(blender);

    auto _simpleblends = get_simpleblends(scope);
    auto itr = _simpleblends.find(blenderid);

    // validate blenderid
    check(itr != _simpleblends.end(), "Burner blend does not exist!");

    // validate scope
    check(itr->collection == scope, "Scope does not own blender!");

    // check if the smart contract is authorized in the collection
    check(isAuthorized(itr->collection, get_self()), "Smart Contract is not authorized for the blend's collection!");

    // check collection mint limit and supply
    auto templates = atomicassets::get_templates(scope);
    auto itrTemplate = templates.require_find(itr->target, "Target template not found from collection!");
    check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Blender cannot mint more assets for the target template id!");

    // get id templates of assets
    vector<uint64_t> ingredients = itr->ingredients;
    vector<uint64_t> blendTemplates = {};
    auto assets = atomicassets::get_assets(get_self());
    auto itrAsset = assets.begin();
    for (size_t i = 0; i < assetids.size(); i++)
    {
        itrAsset = assets.find(assetids[i]);
        blendTemplates.push_back(itrAsset->template_id);
    }

    // verify if assets match with the ingredients
    sort(blendTemplates.begin(), blendTemplates.end());
    sort(ingredients.begin(), ingredients.end());

    check(blendTemplates == ingredients, "Invalid ingredients!");

    // time to blend and burn
    mintasset(itr->collection, itrTemplate->schema_name, itr->target, blender);
    burnassets(assetids);

    // remove assets from nftrefunds
    removeRefundNFTs(blender, scope, assetids);
}

/**
 * Call Simple Swap
*/
ACTION shomaiiblend::callswsimple(uint64_t blenderid, name blender, name scope, uint64_t assetid)
{
    require_auth(blender);
    blockContract(blender);

    auto _simpleswaps = get_simpleswaps(scope);
    auto itr = _simpleswaps.find(blenderid);

    // validate blenderid
    check(itr != _simpleswaps.end(), "Swapper blend does not exist!");

    // validate scope
    check(itr->collection == scope, "Scope does not own blender!");

    // check if the smart contract is authorized in the collection
    check(isAuthorized(itr->collection, get_self()), "Smart Contract is not authorized for the blend's collection!");

    // check collection mint limit and supply
    auto templates = atomicassets::get_templates(scope);
    auto itrTemplate = templates.require_find(itr->target, "Target template not found from collection!");
    check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Blender cannot mint more assets for the target template id!");

    // get id template of asset
    auto assets = atomicassets::get_assets(get_self());
    auto itrSwap = assets.find(assetid);

    check(itrSwap != assets.end(), "Cannot find template ingredient of asset!.");

    // verify if ingredients include the swap template
    check(itr->ingredient == uint64_t(itrSwap->template_id), "Invalid ingredient for swap!");

    // time to swap and burn
    mintasset(itr->collection, itrTemplate->schema_name, itr->target, blender);
    vector<uint64_t> assetids = {assetid};
    burnassets(assetids);

    // remove nfts from refund
    removeRefundNFTs(blender, scope, assetids);
}

ACTION shomaiiblend::callblslot(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids)
{
    require_auth(blender);

    auto _slotblends = get_slotblends(scope);

    auto itrBlender = _slotblends.require_find(blenderid, "Slot Blender does not exist!");

    // CHECK ingredients in here

    // https://github.com/pinknetworkx/atomicpacks-contract/blob/master/src/unboxing.cpp#L206
    // Get signing value from transaction id
    size_t size = transaction_size();
    char buf[size];
    int32_t read = read_transaction(buf, size);
    check(size == read, "Signing values generation: read_transaction() has failed.");
    checksum256 txid = sha256(buf, read);
    uint64_t signing_value;
    memcpy(&signing_value, txid.data(), sizeof(signing_value));

    //Check if the signing_value was already used.
    //If that is the case, increment the signing_value until a non-used value is found
    while (orng::signvals.find(signing_value) != orng::signvals.end())
    {
        signing_value++;
    }

    // Get claim counter
    config_s current_config = config.get();
    uint64_t claim_id = current_config.claimcounter++;
    config.set(current_config, get_self());

    // save job
    claimjobs.emplace(get_self(), [&](claimjob_s &row)
                      {
                          row.claim_id = claim_id;

                          row.blender = blender;
                          row.blenderid = blenderid;
                          row.scope = scope;

                          row.assets = assetids;
                      });

    action(
        permission_level{get_self(), name("active")},
        orng::ORNG_CONTRACT,
        name("requestrand"),
        make_tuple(
            claim_id,
            signing_value,
            get_self()))
        .send();
}
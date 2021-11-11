#include <shomaiiblend.hpp>

/**
 * Call Simple Blend.
*/
ACTION shomaiiblend::callblsimple(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids) {
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
    vector<uint32_t> ingredients = itr->ingredients;
    vector<uint32_t> blendTemplates = {};
    auto assets = atomicassets::get_assets(get_self());
    auto itrAsset = assets.begin();
    for (size_t i = 0; i < assetids.size(); i++) {
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
ACTION shomaiiblend::callswsimple(uint64_t blenderid, name blender, name scope, uint64_t assetid) {
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

/**
 * Call slot blend.
*/
ACTION shomaiiblend::callblslot(uint64_t blenderid, name blender, name scope, vector<uint64_t> assetids, uint64_t claim_id) {
    require_auth(blender);

    // check claim_id very first
    check(claimjobs.find(claim_id) == claimjobs.end(), "Generate another unique claim id!");

    auto _slotblends = get_slotblends(scope);

    auto itrBlender = _slotblends.require_find(blenderid, "Slot Blender does not exist!");

    int lastIndex = 0;

    // CHECK ingredients in here
    for (auto &j : itrBlender->ingredients) {
        switch (j.props.index()) {
            case 0: {
                // check if they have the required schemas

                auto k = get<SlotBlendSchemaIngredient>(j.props);

                for (int i = 0; i < j.amount; i++) {
                    auto asset = assetids[lastIndex];
                    auto itr = validateasset(asset, blender);
                    check(itr->schema_name == k.schema, "The asset ingredient is not from the required slot schema!");

                    lastIndex++;
                }

                break;
            }
            case 1: {
                auto k = get<SlotBlendTemplateIngredient>(j.props);

                for (int i = 0; i < j.amount; i++) {
                    auto asset = assetids[lastIndex];
                    auto itr = validateasset(asset, blender);

                    bool ok = false;

                    for (const auto &x : k.templates) {
                        if (itr->template_id == x) {
                            ok = true;

                            // stop loop once true
                            break;
                        }
                    }

                    check(ok, "The asset ingredient does not meet the required templates for blending!");

                    lastIndex++;
                }

                break;
            }
            case 2: {
                auto k = get<SlotBlendAttribIngredient>(j.props);

                auto itrSchemas = atomicassets::get_schemas(j.collection);
                auto itrTemplates = atomicassets::get_templates(j.collection);

                auto itrSchema = itrSchemas.require_find(k.schema.value, "Schema does not exist in the ingredient's collection!");

                for (int i = 0; i < j.amount; i++) {
                    auto asset = assetids[lastIndex];
                    auto itr = validateasset(asset, blender);
                    auto assetTemplate = itrTemplates.find(uint64_t(itr->template_id));

                    atomicassets::ATTRIBUTE_MAP temp_data = atomicdata::deserialize(assetTemplate->immutable_serialized_data, itrSchema->format);

                    bool ok = false;

                    for (const auto &x : k.attributes) {
                        for (const auto &y : x.allowed_values) {
                            auto value = get<string>(temp_data[x.key]);

                            // check if attribute value includes the allowed_value
                            if (value.find(y) != string::npos) {
                                ok = true;

                                if (!k.require_all_attribs) {
                                    break;
                                }
                            } else {
                                ok = false;
                            }
                        }

                        if (ok && !k.require_all_attribs) {
                            break;
                        }
                    }

                    check(ok, "The asset ingredient does not meet the required schema attributes.");

                    lastIndex++;
                }

                break;
            }
            default: {
                check(false, "Invalid ingredient type!");
            }
        }
    }

    // check if there is only one target
    auto blender_targets = get_blendertargets(scope);
    auto itr_blender_targets = blender_targets.require_find(blenderid, "Blender's target pool does not exist.");

    // if only one target, just mint and burn
    if (itr_blender_targets->targets.size() == 1) {
        auto _target = itr_blender_targets->targets[0];
        auto _templates = atomicassets::get_templates(scope);

        auto itr_target = _templates.require_find(_target.templateid, "Target template of blender does not exist!");

        // time to swap and burn
        mintasset(scope, itr_target->schema_name, _target.templateid, blender);
        burnassets(assetids);

        // remove nfts from refund
        removeRefundNFTs(blender, scope, assetids);

        return;
    }

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
    while (orng::signvals.find(signing_value) != orng::signvals.end()) {
        signing_value++;
    }

    // save job
    claimjobs.emplace(get_self(), [&](claimjob_s &row) {
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
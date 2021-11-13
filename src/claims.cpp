#include <shomaiiblend.hpp>

#include "randomness_provider.cpp"

ACTION shomaiiblend::receiverand(uint64_t assoc_id, checksum256 random_value) {
    require_auth(orng::ORNG_CONTRACT);

    RandomnessProvider random_provider(random_value);

    auto claimjob = claimjobs.find(assoc_id);

    auto targetstable = get_blendertargets(claimjob->scope);
    auto _target = targetstable.find(claimjob->blenderid);

    auto claimassets = get_claimassets(claimjob->scope);

    // get random  https://github.com/pinknetworkx/atomicpacks-contract/blob/master/src/unboxing.cpp#L133-L147
    uint32_t rand = random_provider.get_rand(TOTALODDS);  // default totalodds to 100
    uint32_t summed_odds = 0;
    for (const MultiTarget &i : _target->targets) {
        summed_odds += i.odds;

        if (summed_odds > rand) {
            claimassets.emplace(get_self(), [&](claimassets_s &row) {
                row.blender = claimjob->blender;
                row.blenderid = claimjob->blenderid;
                row.claim_id = claimjob->claim_id;
                row.templateid = i.templateid;
                row.assets = claimjob->assets;
            });

            break;
        }
    }

    // remove the assets if blend has been added to claims
    removeRefundNFTs(claimjob->blender, claimjob->scope, claimjob->assets);

    // erase the job
    claimjobs.erase(claimjob);
}

ACTION shomaiiblend::claimblslot(uint64_t claim_id, name blender, name scope) {
    require_auth(blender);

    auto claimassets = get_claimassets(scope);

    auto itrClaim = claimassets.require_find(claim_id, "Claim ID does not exist, maybe it was already claimed?");
    check(itrClaim->blender == blender, "Claim blender is not similar with the caller!");  // check if claimer is same with blender, unnecessary?

    // get claim template
    auto itrTemplate = get_target_template(scope, uint64_t(itrClaim->templateid));

    mintasset(scope, itrTemplate->schema_name, itrClaim->templateid, blender);
    burnassets(itrClaim->assets);
}
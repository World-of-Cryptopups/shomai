#include <shomaiiblend.hpp>

/**
 * Checks and validates the blend config.
*/
void shomaiiblend::check_config(uint64_t blenderid, name blender, name scope) {
    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.find(blenderid);

    // do not check if no config set
    if (itrConfig == _blendconfig.end()) return;

    // check the dates
    check(now() >= itrConfig->startdate, "Still waiting for start date.");
    check(now() <= itrConfig->enddate, "Blending end date has already passed.");

    // check the whitelist
    if (itrConfig->enable_whitelists) {
        check(find(itrConfig->whitelists.begin(), itrConfig->whitelists.end(), blender) != itrConfig->whitelists.end(), "You are not whitelisted for this blend.");
    }

    // check the max uses
    check(itrConfig->maxuse != 0, "The max use of the blend is currently zero.");

    // check the max user use
    auto _blenduses = get_userblends(blender);
    auto itrBlendUses = _blenduses.find(blenderid);
    if (itrBlendUses != _blenduses.end()) {
        check(itrBlendUses->total_uses <= itrConfig->maxuseruse, "Max user use has been reached!");
    }
}

/**
 * This action is set to enable / disable a blend config.
 * If enabled, it will initialize a new config, otherwise, it will be erased.
*/
ACTION shomaiiblend::setconfig(name author, uint64_t blenderid, name scope, bool enable) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.find(blenderid);

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    // if enable == false, remove the config
    if (!enable) {
        check(itrConfig != _blendconfig.end(), "Config for blend is already not enabled.");

        _blendconfig.erase(itrConfig);

        return;
    }

    check(itrConfig == _blendconfig.end(), "Config for blend is already enabled.");

    _blendconfig.emplace(author, [&](blendconfig_s &row) {
        row.blenderid = blenderid;
        row.maxuse = 0;
        row.maxuseruse = 0;
        row.startdate = -1;
        row.enddate = -1;
        row.enable_whitelists = false;
        row.whitelists = {};
    });
}

/**
 * Set the whitelists. This will set the `names_list` to the `whitelists` and no other checking is done.
*/
ACTION shomaiiblend::setwhitelist(name author, uint64_t blenderid, name scope, vector<name> names_list) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.require_find(blenderid, "Config for blend does not exist!");

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) {
        row.whitelists = names_list;
    });
}

/**
 * Enable the blend whitelisting.
*/
ACTION shomaiiblend::setonwhlist(name author, uint64_t blenderid, name scope, bool on_whitelist) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.require_find(blenderid, "Config for blend does not exist!");

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) {
        row.enable_whitelists = on_whitelist;
    });
}

/**
 * Set the blend startdate and enddate.
 * If value is 0, it won't be updated.
*/
ACTION shomaiiblend::setdates(name author, uint64_t blenderid, name scope, int32_t startdate, int32_t enddate) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.require_find(blenderid, "Config for blend does not exist!");

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    if (enddate != 0 && enddate != -1 && startdate != 0 && startdate != -1) {
        check(now() < startdate, "Start date should be greater than now.");
        check(now() < enddate, "End date should be greater than now.");
        check(enddate > startdate, "End date should be greater than the startdate.");
        }

    _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) {
        if (startdate != 0) {
            row.startdate = startdate;
        }
        if (enddate != 0) {
            row.enddate = enddate;
        }
    });
}

/**
 * Set the maximum blend uses.
 * If `maxuse` is 0, the maxuseruse will not be updated.
*/
ACTION shomaiiblend::setmax(name author, uint64_t blenderid, name scope, int32_t maxuse, int32_t maxuseruse) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.require_find(blenderid, "Config for blend does not exist!");

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) {
        row.maxuse = maxuse;
        row.maxuseruse = maxuseruse;
    });
};
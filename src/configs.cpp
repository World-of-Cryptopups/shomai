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
    if (itrConfig->startdate != -1) {
        check(now() >= itrConfig->startdate, "Still waiting for start date.");
    }
    if (itrConfig->enddate != -1) {
        check(now() <= itrConfig->enddate, "Blending end date has already passed.");
    }

    // check the whitelist
    if (itrConfig->enable_whitelists) {
        check(find(itrConfig->whitelists.begin(), itrConfig->whitelists.end(), blender) != itrConfig->whitelists.end(), "You are not whitelisted for this blend.");
    }

    // check the max uses
    check(itrConfig->maxuse != 0, "The max use of the blend is currently zero.");
    auto _blendstats = get_blendstats(scope);
    auto itrBlendStats = _blendstats.find(blenderid);
    if (itrBlendStats != _blendstats.end()) {
        // check total uses
        check(itrConfig->maxuse >= itrBlendStats->total_uses, "Maximum blend total use limit reached.");
    }

    // check the max user use
    auto _blenduses = get_userblends(blender);
    auto itrBlendUses = _blenduses.find(blenderid);
    if (itrBlendUses != _blenduses.end()) {
        // check maximum user use
        check(itrBlendUses->uses <= itrConfig->maxuseruse, "Max user use has been reached!");

        // check cooldown
        check(now() - itrBlendUses->last_used > itrConfig->maxusercooldown, "Blend use is still in cooldown.");
    }
}

/**
 * This is called when removing a blend.
 * An authorization check should be called before calling this one.
 * The difference with setconfig is that it checks if the config is set or not and raises an error.
*/
void shomaiiblend::remove_blend_config(uint64_t blenderid, name author, name scope) {
    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.find(blenderid);

    // do not remove if no config set
    if (itrConfig == _blendconfig.end()) return;

    // erase the config
    _blendconfig.erase(itrConfig);
}

/**
 * This increments the blend total use and the blender's use.
*/
void shomaiiblend::increment_blend_use(uint64_t blenderid, name blender, name scope) {
    auto _blendstats = get_blendstats(scope);
    auto itrBlendstats = _blendstats.find(blenderid);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.find(blenderid);

    if (itrBlendstats == _blendstats.end()) {
        // addd new stats info if it doesn't exist
        _blendstats.emplace(blender, [&](blendstats_s &row) {
            row.blenderid = blenderid;
            row.total_uses = 1;
        });
    } else {
        // update stats
        _blendstats.modify(itrBlendstats, blender, [&](blendstats_s &row) {
            row.total_uses = itrBlendstats->total_uses + 1;
        });
    }

    // update or create the user's blend use on the blend if the maxuserblend is not infinite
    if (itrConfig != _blendconfig.end()) {
        if (itrConfig->maxusercooldown != -1) {
            // update only if cooldown is now infinite

            auto _blenduses = get_userblends(blender);
            auto itrBlendUses = _blenduses.find(blenderid);

            if (itrBlendUses != _blenduses.end()) {
                _blenduses.emplace(blender, [&](blendconfiguses_s &row) {
                    row.blenderid = blenderid;
                    row.blender = blender;
                    row.last_used = now();
                    row.uses = 1;
                });
            } else {
                _blenduses.modify(itrBlendUses, blender, [&](blendconfiguses_s &row) {
                    row.last_used = now();
                    row.uses = itrBlendUses->uses + 1;
                });
            }
        }
    }
}

/**
 * This action is set to enable / disable a blend config.
 * If enabled, it will initialize a new config, otherwise, it will be erased.
*/
ACTION shomaiiblend::removeconfig(name author, uint64_t blenderid, name scope) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.require_find(blenderid, "Config of blend does not exist!");

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    // remove blend config
    _blendconfig.erase(itrConfig);
}

/**
 * Set the whitelists. This will set the `names_list` to the `whitelists` and no other checking is done.
*/
ACTION shomaiiblend::setwhitelist(name author, uint64_t blenderid, name scope, vector<name> names_list) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.find(blenderid);

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    if (itrConfig == _blendconfig.end()) {
        _blendconfig.emplace(author, [&](blendconfig_s &row) {
            row.blenderid = blenderid;
            row.whitelists = names_list;
        });
        return;
    }

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
    auto itrConfig = _blendconfig.find(blenderid);

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    if (itrConfig == _blendconfig.end()) {
        _blendconfig.emplace(author, [&](blendconfig_s &row) {
            row.blenderid = blenderid;
            row.enable_whitelists = on_whitelist;
        });
        return;
    }

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
    auto itrConfig = _blendconfig.find(blenderid);

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    if (enddate != 0 && enddate != -1 && startdate != 0 && startdate != -1) {
        if (itrConfig->startdate != startdate) {
            check(now() < startdate, "Start date should be greater than now.");
        }

        if (itrConfig->enddate != enddate) {
            check(now() < enddate, "End date should be greater than now.");
        }

        check(enddate > startdate, "End date should be greater than the startdate.");
    }

    if (itrConfig == _blendconfig.end()) {
        _blendconfig.emplace(author, [&](blendconfig_s &row) {
            row.blenderid = blenderid;
            row.startdate = startdate;
            row.enddate = enddate;
        });
        return;
    }

    _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) {
        row.startdate = startdate;
        row.enddate = enddate;
    });
}

/**
 * Set the maximum blend uses.
 * If `maxuse` is 0, the maxuseruse will not be updated.
*/
ACTION shomaiiblend::setmax(name author, uint64_t blenderid, name scope, int32_t maxuse, int32_t maxuseruse, int32_t maxusercooldown) {
    require_auth(author);
    blockContract(author);

    auto _blendconfig = get_blendconfigs(scope);
    auto itrConfig = _blendconfig.find(blenderid);

    check(isAuthorized(scope, author), "User is not authorized in collection!");

    if (itrConfig == _blendconfig.end()) {
        _blendconfig.emplace(author, [&](blendconfig_s &row) {
            row.blenderid = blenderid;
            row.maxuse = maxuse;
            row.maxuseruse = maxuseruse;
            row.maxusercooldown = maxusercooldown;
        });
        return;
    }

    _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) {
        row.maxuse = maxuse;
        row.maxuseruse = maxuseruse;
        row.maxusercooldown = maxusercooldown;
    });
};
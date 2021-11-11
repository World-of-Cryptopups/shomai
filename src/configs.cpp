#include <shomaiiblend.hpp>

/**
 * Set Blend Config of a simple blend.
*/
ACTION shomaiiblend::setblsimconf(name author, uint64_t blenderid, name scope, BlendConfig config) {
    require_auth(author);
    blockContract(author);

    auto _simpleblends = get_simpleblends(scope);
    auto _blendconfig = get_blendconfigs(scope);

    auto itr = _simpleblends.find(blenderid);
    auto itrConfig = _blendconfig.find(blenderid);

    // check if user is authorized in the collection
    check(isAuthorized(itr->collection, author), "User is not authorized for this collection!");

    if (itrConfig == _blendconfig.end()) {
        // no config set yet
        _blendconfig.emplace(author, [&](blendconfig_s &row) {
            row.blenderid = blenderid;
            row.config = config;
        });
    } else {
        // modify the existing config
        _blendconfig.modify(itrConfig, author, [&](blendconfig_s &row) { row.config = config; });
    }
}

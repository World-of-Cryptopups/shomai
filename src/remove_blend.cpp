#include <shomaiiblend.hpp>

/**
 * Remove a Simple Blend.
 * User should be authorized by the collection blender.
*/
ACTION shomaiiblend::remblsimple(name user, name scope, uint64_t blenderid) {
    require_auth(user);
    blockContract(user);

    auto _simpleblends = get_simpleblends(scope);
    auto itr = _simpleblends.require_find(blenderid, "Burner ID does not exist!");

    // check if user is authorized in collection
    check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

    // remove item
    _simpleblends.erase(itr);

    // remove blend config if it exists
    remove_blend_config(blenderid, user, scope);

    // remove blend stats if it exists
    remove_blend_stats(blenderid, user, scope);
}

/**
 * Remove a simple swap.
 * User should be authorized to do this.
*/
ACTION shomaiiblend::remswsimple(name user, name scope, uint64_t blenderid) {
    require_auth(user);
    blockContract(user);

    auto _simpleswaps = get_simpleswaps(scope);
    auto itr = _simpleswaps.require_find(blenderid, "Swapper ID does not exist!");

    // check if blenderid author/user is user
    check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

    // remove item
    _simpleswaps.erase(itr);

    // remove blend config if it exists
    remove_blend_config(blenderid, user, scope);

    // remove blend stats if it exists
    remove_blend_stats(blenderid, user, scope);
}

/**
 * Remove a Slot Blend.
 * User should be authorized by the collection blender.
*/
ACTION shomaiiblend::remblslot(name user, name scope, uint64_t blenderid) {
    require_auth(user);
    blockContract(user);

    auto _slotblends = get_slotblends(scope);
    auto itr = _slotblends.require_find(blenderid, "Burner ID does not exist!");

    // check if user is authorized in collection
    check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

    // remove item
    _slotblends.erase(itr);

    // remove blend config if it exists
    remove_blend_config(blenderid, user, scope);

    // remove blend stats if it exists
    remove_blend_stats(blenderid, user, scope);
}
#include <shomaiiblend.hpp>

/**
 * Remove a Simple Blend.
 * User should be authorized by the collection blender.
*/
ACTION shomaiiblend::remblsimple(name user, name scope, uint64_t blenderid)
{
    require_auth(user);
    blockContract(user);

    auto _simpleblends = get_simpleblends(scope);
    auto itr = _simpleblends.find(blenderid);

    // check if blenderid exists
    check(itr != _simpleblends.end(), "Burner ID does not exist!");

    // check if user is authorized in collection
    check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

    // remove item
    _simpleblends.erase(itr);
}

/**
 * Remove a simple swap.
 * User should be authorized to do this.
*/
ACTION shomaiiblend::remswsimple(name user, name scope, uint64_t blenderid)
{
    require_auth(user);
    blockContract(user);

    auto _simpleswaps = get_simpleswaps(scope);
    auto itr = _simpleswaps.find(blenderid);

    // check if blenderid exists
    check(itr != _simpleswaps.end(), "Swapper ID does not exist!");

    // check if blenderid author/user is user
    check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

    // remove item
    _simpleswaps.erase(itr);
}
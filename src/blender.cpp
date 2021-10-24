#include <blender.hpp>

ACTION blender::makeburner(name user, name targetcol, name targettemp, vector<uint64_t> ingredients)
{
  require_auth(user);
}

ACTION blender::delburner(name user, uint64_t burnerid)
{
  require_auth(user);

  auto itr = burner.find(burnerid);

  // check if burnerid exists
  check(itr != burner.end(), "Burner ID does not exist!");

  // check if burnerid author/user is user
  check(itr->user == user, "Burner ID does is not owned by user!");

  // remove item
  burner.erase(itr);
}

ACTION blender::makeswapper(name user, name targetcol, name targettemp, vector<uint64_t> ingredients)
{
  require_auth(user);
}

ACTION blender::delswapper(name user, uint64_t swapperid)
{
  require_auth(user);

  auto itr = swapper.find(swapperid);

  // check if swapperid exists
  check(itr != swapper.end(), "Swapper ID does not exist!");

  // check if swapperid author/user is user
  check(itr->user == user, "Swapper ID does is not owned by user!");

  // remove item
  swapper.erase(itr);
}

ACTION blender::blendburner(name blender, vector<uint64_t> assetids)
{
  require_auth(blender);

  // do not allow the smart contract to blend for itself
  check(blender != _self, "The smart contract itself should not be able to blend!");
}

ACTION blender::blendswapper(name blender, vector<uint64_t> assetids)
{
  require_auth(blender);

  // do not allow the smart contract to blend for itself
  check(blender != _self, "The smart contract itself should not be able to blend!");
}

EOSIO_DISPATCH(blender, (makeburner)(delburner)(makeswapper)(delswapper)(blendburner)(blendswapper))

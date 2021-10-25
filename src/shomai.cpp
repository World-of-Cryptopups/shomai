#include <blender.hpp>

/*
Initialize singleton db.
*/
ACTION blender::init()
{
  require_auth(_self);

  config.get_or_create(_self, config_s{});
}

/*
Create a `burner` blend.
*/
ACTION blender::makeblender(name user, name targetcol, uint64_t targettemp, vector<uint64_t> ingredients)
{
  require_auth(user);

  // validate target collection
  auto itrCol = atomicassets::collections.require_find(targetcol.value, "This collection does not exist!");

  // validate user if authorized in collection
  check(isAuthorized(targetcol, user), "You are not authorized in this collection!");

  // validate contract if authorized by collection
  check(isAuthorized(targetcol, _self), "Contract is not authorized in the collection!");

  // get target collection
  atomicassets::templates_t templates = atomicassets::templates_t(ATOMICASSETS, targetcol.value);

  // validate template if exists in collection
  check(templates.find(targettemp) != templates.end(), "Template does not exist in collection!");

  // get burner counter
  config_s current_config = config.get();
  uint64_t blenderid = current_config.burnercounter++;
  config.set(current_config, _self);

  // create blend info
  blenders.emplace(user, [&](auto &row)
                   {
                     row.blenderid = blenderid;
                     row.user = user;
                     row.targetcol = targetcol;
                     row.targettemp = targettemp;
                     row.ingredients = ingredients;
                   });
}

/*
  Remove a blend.
*/
ACTION blender::remblend(name user, uint64_t blenderid)
{
  require_auth(user);

  auto itr = blenders.find(blenderid);

  // check if blenderid exists
  check(itr != blenders.end(), "Burner ID does not exist!");

  // check if blenderid author/user is user
  check(itr->user == user, "Burner ID does is not owned by user!");

  // remove item
  blenders.erase(itr);
}

/*
  Create a swap.
*/
ACTION blender::makeswapper(name user, name targetcol, uint64_t targettemp, vector<uint64_t> ingredients)
{
  require_auth(user);

  // validate target collection
  auto itrCol = atomicassets::collections.require_find(targetcol.value, "This collection does not exist!");

  // validate user if authorized in collection
  check(isAuthorized(targetcol, user), "You are not authorized in this collection!");

  // validate contract if authorized by collection
  check(isAuthorized(targetcol, _self), "Contract is not authorized in the collection!");

  // get target collection
  atomicassets::templates_t templates = atomicassets::templates_t(name("atomicassets"), targetcol.value);

  // validate template if exists in collection
  check(templates.find(targettemp) != templates.end(), "Template does not exist in collection!");

  // get burner counter
  config_s current_config = config.get();
  uint64_t swapperid = current_config.swappercounter++;
  config.set(current_config, _self);

  // create blend info
  swappers.emplace(user, [&](auto &row)
                   {
                     row.swapperid = swapperid;
                     row.user = user;
                     row.targetcol = targetcol;
                     row.targettemp = targettemp;
                     row.ingredients = ingredients;
                   });
}

/*
Remove a swap.
*/
ACTION blender::remswap(name user, uint64_t swapperid)
{
  require_auth(user);

  auto itr = swappers.find(swapperid);

  // check if swapperid exists
  check(itr != swappers.end(), "Swapper ID does not exist!");

  // check if swapperid author/user is user
  check(itr->user == user, "Swapper ID does is not owned by user!");

  // remove item
  swappers.erase(itr);
}

/*
Blend assets.
*/
ACTION blender::callblender(uint64_t blenderid, name blender, vector<uint64_t> assetids)
{
  require_auth(blender);

  // do not allow the smart contract to blend for itself
  check(blender != _self, "The smart contract itself should not be able to blend!");

  auto itr = blenders.find(blenderid);

  // validate blenderid
  check(itr != blenders.end(), "Burner blend does not exist!");

  // if not unlimited, check blend max use
  if (itr->maxuse != -1)
  {
    check(itr->maxuse != itr->uses, "Maximum blend use has been reached!");
  }

  // check if the smart contract is authorized in the collection
  check(isAuthorized(itr->targetcol, _self), "Smart Contract is not authorized for the blend's collection!");

  // check collection mint limit and supply
  atomicassets::templates_t templates = atomicassets::templates_t(ATOMICASSETS, itr->targetcol.value);
  auto itrTemplate = templates.require_find(itr->targettemp, "Target template not found from collection!");
  check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Blender cannot mint more assets for the target template id!");

  // get id templates of assets
  vector<uint64_t> ingredients = itr->ingredients;
  vector<uint64_t> blendTemplates = {};
  atomicassets::assets_t assets = atomicassets::assets_t(ATOMICASSETS, get_self().value);
  auto itrAsset = assets.begin();
  for (size_t i = 0; i < assetids.size(); i++)
  {
    itrAsset = assets.find(assetids[i]);
    blendTemplates.push_back(itrAsset->template_id);
  }

  // verify if assets match with the ingredients
  sort(blendTemplates.begin(), blendTemplates.end());
  sort(ingredients.begin(), ingredients.end());
  check(blendTemplates == ingredients, "Invalid asset ingradients!");

  // time to blend and burn
  mintasset(itr->targetcol, itrTemplate->schema_name, itr->targettemp, blender);
  burnassets(blender, assetids);

  // update use
  blenders.modify(itr, itr->user, [&](auto &row)
                  { row.uses = itr->uses + 1; });
}

/*
Swap assets.
*/
ACTION blender::callswap(uint64_t swapperid, name blender, uint64_t assetid)
{
  require_auth(blender);

  // do not allow the smart contract to blend for itself
  check(blender != _self, "The smart contract itself should not be able to blend!");

  auto itr = swappers.find(swapperid);

  // validate swapperid
  check(itr != swappers.end(), "Swapper blend does not exist!");

  // if not unlimited, check swap max use
  if (itr->maxuse != -1)
  {
    check(itr->maxuse != itr->uses, "Maximum swap use has been reached!");
  }

  // check if the smart contract is authorized in the collection
  check(isAuthorized(itr->targetcol, _self), "Smart Contract is not authorized for the blend's collection!");

  // check collection mint limit and supply
  atomicassets::templates_t templates = atomicassets::templates_t(ATOMICASSETS, itr->targetcol.value);
  auto itrTemplate = templates.require_find(itr->targettemp, "Target template not found from collection!");
  check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Blender cannot mint more assets for the target template id!");

  // get id template of asset
  vector<uint64_t> ingredients = itr->ingredients;
  atomicassets::assets_t assets = atomicassets::assets_t(ATOMICASSETS, _self.value);
  uint64_t swapTemplate = assets.find(assetid)->template_id;

  // verify if ingredients include the swap template
  check(find(ingredients.begin(), ingredients.end(), swapTemplate) != ingredients.end(), "Invalid ingredient for swap!");

  // time to swap and burn
  mintasset(itr->targetcol, itrTemplate->schema_name, itr->targettemp, blender);
  burnassets(blender, assetid);

  // update use
  swappers.modify(itr, itr->user, [&](auto &row)
                  { row.uses = itr->uses + 1; });
}

EOSIO_DISPATCH(blender, (init)(makeblender)(remblend)(makeswapper)(remswap)(callblender)(callswap))

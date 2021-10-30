#include <shomaiblendx.hpp>

/*
Initialize singleton db.
*/
ACTION shomaiblendx::init()
{
  require_auth(_self);

  config.get_or_create(_self, config_s{});
}

/**
 * Create a Simple Blend (same collection only)
*/
ACTION shomaiblendx::makeblsimple(name author, name collection, uint64_t target, vector<uint64_t> ingredients)
{
  require_auth(author);

  // validate target collection
  auto itrCol = atomicassets::collections.require_find(collection.value, "This collection does not exist!");

  // validate author if authorized in collection
  check(isAuthorized(collection, author), "You are not authorized in this collection!");

  // validate contract if authorized by collection
  check(isAuthorized(collection, _self), "Contract is not authorized in the collection!");

  // get target collection
  atomicassets::templates_t templates = atomicassets::templates_t(ATOMICASSETS, collection.value);

  // validate template if exists in collection
  check(templates.find(target) != templates.end(), "Template does not exist in collection!");
  for (uint64_t i : ingredients)
  {
    check(templates.find(i) != templates.end(), "Template ingredient does not exist in collection!");
  }

  // get burner counter
  config_s current_config = config.get();
  uint64_t blenderid = current_config.blendercounter++;
  config.set(current_config, _self);

  // create blend info
  simblends.emplace(author, [&](simpleblend_s &row)
                    {
                      row.blenderid = blenderid;
                      row.author = author;
                      row.collection = collection;
                      row.target = target;
                      row.ingredients = ingredients;
                    });
}

/**
 * Remove a Simple Blend.
 * User should be authorized by the collection blender.
*/
ACTION shomaiblendx::remblsimple(name user, uint64_t blenderid)
{
  require_auth(user);

  auto itr = simblends.find(blenderid);

  // check if blenderid exists
  check(itr != simblends.end(), "Burner ID does not exist!");

  // check if user is authorized in collection
  check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

  // remove item
  simblends.erase(itr);
}

/**
 * Create a Simple Swap. (same collection only)
*/
ACTION shomaiblendx::makeswsimple(name author, name collection, uint64_t target, uint64_t ingredient)
{
  require_auth(author);

  // validate target collection
  auto itrCol = atomicassets::collections.require_find(collection.value, "This collection does not exist!");

  // validate author if authorized in collection
  check(isAuthorized(collection, author), "You are not authorized in this collection!");

  // validate contract if authorized by collection
  check(isAuthorized(collection, _self), "Contract is not authorized in the collection!");

  // get target collection
  atomicassets::templates_t templates = atomicassets::templates_t(name("atomicassets"), collection.value);

  // validate template if exists in collection
  check(templates.find(target) != templates.end(), "Template does not exist in collection!");

  // get burner counter
  config_s current_config = config.get();
  uint64_t blenderid = current_config.blendercounter++;
  config.set(current_config, _self);

  // create blend info
  simswaps.emplace(author, [&](simpleswap_s &row)
                   {
                     row.blenderid = blenderid;
                     row.author = author;
                     row.collection = collection;
                     row.target = target;
                     row.ingredient = ingredient;
                   });
}

/**
 * Remove a simple swap.
 * User should be authorized to do this.
*/
ACTION shomaiblendx::remswsimple(name user, uint64_t blenderid)
{
  require_auth(user);

  auto itr = simswaps.find(blenderid);

  // check if blenderid exists
  check(itr != simswaps.end(), "Swapper ID does not exist!");

  // check if blenderid author/user is user
  check(isAuthorized(itr->collection, user), "User is not authorized in this collection!");

  // remove item
  simswaps.erase(itr);
}

/**
 * Call Simple Blend.
*/
ACTION shomaiblendx::callblsimple(uint64_t blenderid, name blender, vector<uint64_t> assetids)
{
  require_auth(blender);

  // do not allow the smart contract to blend for itself
  check(blender != _self, "The smart contract itself should not be able to blend!");

  auto itr = simblends.find(blenderid);

  // validate blenderid
  check(itr != simblends.end(), "Burner blend does not exist!");

  // check if the smart contract is authorized in the collection
  check(isAuthorized(itr->collection, _self), "Smart Contract is not authorized for the blend's collection!");

  // check collection mint limit and supply
  atomicassets::templates_t templates = atomicassets::templates_t(ATOMICASSETS, itr->collection.value);
  auto itrTemplate = templates.require_find(itr->target, "Target template not found from collection!");
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
  mintasset(itr->collection, itrTemplate->schema_name, itr->target, blender);
  burnassets(blender, assetids);
}

/**
 * Call Simple Swap
*/
ACTION shomaiblendx::callswsimple(uint64_t blenderid, name blender, uint64_t assetid)
{
  require_auth(blender);

  // do not allow the smart contract to blend for itself
  check(blender != _self, "The smart contract itself should not be able to blend!");

  auto itr = simswaps.find(blenderid);

  // validate blenderid
  check(itr != simswaps.end(), "Swapper blend does not exist!");

  // check if the smart contract is authorized in the collection
  check(isAuthorized(itr->collection, _self), "Smart Contract is not authorized for the blend's collection!");

  // check collection mint limit and supply
  atomicassets::templates_t templates = atomicassets::templates_t(ATOMICASSETS, itr->collection.value);
  auto itrTemplate = templates.require_find(itr->target, "Target template not found from collection!");
  check(itrTemplate->max_supply > itrTemplate->issued_supply || itrTemplate->max_supply == 0, "Blender cannot mint more assets for the target template id!");

  // get id template of asset
  atomicassets::assets_t assets = atomicassets::assets_t(ATOMICASSETS, _self.value);
  uint64_t swapTemplate = assets.find(assetid)->template_id;

  // verify if ingredients include the swap template
  check(itr->ingredient == swapTemplate, "Invalid ingredient for swap!");

  // time to swap and burn
  mintasset(itr->collection, itrTemplate->schema_name, itr->target, blender);
  burnassets(blender, assetid);
}

/**
 * Set Blend Config of a simple blend.
*/
ACTION shomaiblendx::setblsimconf(name author, uint64_t blenderid, BlendConfig config)
{
  require_auth(author);

  auto itr = simblends.find(blenderid);
  auto itrConfig = blendconfigs.find(blenderid);

  // check if user is authorized in the collection
  check(isAuthorized(itr->collection, author), "User is not authorized for this collection!");

  if (itrConfig == blendconfigs.end())
  {
    // no config set yet
    blendconfigs.emplace(author, [&](blendconfig_s &row)
                         {
                           row.blenderid = blenderid;

                           row.config = config;
                         });
  }
  else
  {
    // modify the existing config
    blendconfigs.modify(itrConfig, author, [&](blendconfig_s &row)
                        { row.config = config; });
  }
}

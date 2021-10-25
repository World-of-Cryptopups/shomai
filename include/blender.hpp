#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "atomicassets.hpp"
#include "atomicdata.hpp"

using namespace std;
using namespace eosio;

#define ATOMICASSETS name("atomicassets")

CONTRACT blender : public contract
{
public:
  using contract::contract;

  /*
    Start Blend Actions
    */
  ACTION makeblender(name user, name targetcol, uint64_t targettemp, vector<uint64_t> ingredients);
  ACTION makeswapper(name user, name targetcol, uint64_t targettemp, vector<uint64_t> ingredients);
  ACTION remblend(name user, uint64_t blenderid);
  ACTION remswap(name user, uint64_t swapperid);
  ACTION callblender(uint64_t blenderid, name blender, vector<uint64_t> assetids);
  ACTION callswap(uint64_t swapperid, name blender, uint64_t assetid);
  /*
    End Blend Actions
    */

  /* 
   Start System actions
    */
  ACTION init();
  /* 
   End System actions
    */

private:
  TABLE blender_s
  {
    uint64_t blenderid;
    name user;
    name targetcol;
    uint64_t targettemp;
    vector<uint64_t> ingredients;
    int64_t maxuse = -1; // -1 = infinite use
    uint64_t uses;

    uint64_t primary_key() const { return blenderid; };
    uint64_t by_user() const { return user.value; };
  };

  TABLE swapper_s
  {
    uint64_t swapperid;
    name user;
    name targetcol;
    uint64_t targettemp;
    vector<uint64_t> ingredients; // could be any of, this has different usecase from blender
    int64_t maxuse = -1;          // -1 = infinite use
    uint64_t uses;

    uint64_t primary_key() const { return swapperid; };
    uint64_t by_user() const { return user.value; };
  };

  TABLE config_s
  {
    uint64_t burnercounter = 100000;
    uint64_t swappercounter = 100000;
  };

  typedef singleton<"config"_n, config_s> config_t;
  typedef multi_index<"config"_n, config_s> config_t_for_abi;
  typedef multi_index<"blenders"_n, blender_s, indexed_by<"user"_n, const_mem_fun<blender_s, uint64_t, &blender_s::by_user>>> blender_t;
  typedef multi_index<"swappers"_n, swapper_s, indexed_by<"user"_n, const_mem_fun<swapper_s, uint64_t, &swapper_s::by_user>>> swapper_t;

  /* Initialize tables */
  config_t config = config_t(_self, _self.value);
  blender_t blenders = blender_t(_self, _self.value);
  swapper_t swappers = swapper_t(_self, _self.value);

  // ======== util functions

  /*
      Check if user is authorized to mint NFTs
   */
  bool isAuthorized(name collection, name user)
  {
    auto itr = atomicassets::collections.require_find(collection.value, "No collection with this name exists!");
    bool authorized = false;
    vector<name> accs = itr->authorized_accounts;
    for (auto it = accs.begin(); it != accs.end() && !authorized; it++)
    {
      if (user == name(*it))
      {
        authorized = true;
      }
    }
    return authorized;
  }

  /*
      Call AtomicAssets contract to mint a new NFT
   */
  void mintasset(name collection, name schema, uint64_t templateid, name to)
  {
    vector<uint64_t> back_tokens;
    atomicassets::ATTRIBUTE_MAP nodata = {};

    // call contract
    action(
        permission_level{_self, name("active")},
        ATOMICASSETS,
        name("mintasset"),
        make_tuple(_self, collection, schema, templateid, to, nodata, nodata, back_tokens))
        .send();
  }

  /*
      Call AtomicAssets contract to burn NFTs
      (multiple assets)
   */
  void burnassets(name user, vector<uint64_t> assets)
  {
    for (auto it = assets.begin(); it != assets.end(); it++)
    {
      action(permission_level{user, name("active")},
             ATOMICASSETS,
             name("burnasset"),
             make_tuple(user, *it))
          .send();
    }
  }

  /*
      Call AtomicAssets contract to burn NFTs
      (single asset)
   */
  void burnassets(name user, uint64_t asset)
  {
    action(permission_level{user, name("active")},
           ATOMICASSETS,
           name("burnasset"),
           make_tuple(user, asset))
        .send();
  }
};

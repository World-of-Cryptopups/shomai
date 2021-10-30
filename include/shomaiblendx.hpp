#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "atomicassets.hpp"
#include "atomicdata.hpp"

using namespace std;
using namespace eosio;

#define ATOMICASSETS name("atomicassets")

struct BlendConfig
{
  int64_t maxuse = -1;     // -1 = (global use) infinite use
  int64_t maxuseruse = -1; // -1 = (user use) infinite use
  uint64_t uses;

  int64_t startdate = -1; // -1, start as soon
  int64_t enddate = -1;   // -1, does not end
};

CONTRACT shomaiblendx : public contract
{
public:
  using contract::contract;

  /*
    Start Blend Actions
    */
  ACTION makeblsimple(name author, name collection, uint64_t target, vector<uint64_t> ingredients);
  ACTION makeswsimple(name author, name collection, uint64_t target, uint64_t ingredient);
  ACTION remblsimple(name user, uint64_t blenderid);
  ACTION remswsimple(name user, uint64_t blenderid);
  ACTION callblsimple(uint64_t blenderid, name blender, vector<uint64_t> assetids);
  ACTION callswsimple(uint64_t blenderid, name blender, uint64_t asset);
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
  /*
  Simple Blend (only from one collection)
*/
  TABLE simpleblend_s
  {
    uint64_t blenderid;
    name author; // author

    name collection;
    uint64_t target;
    vector<uint64_t> ingredients; // ingredients should only be from collection

    uint64_t primary_key() const { return blenderid; };
  };

  /*
  Multi Blend (cross-collection, )
  */

  /*
  Simple Swap (swap assets, single collection)
*/
  TABLE simpleswap_s
  {
    uint64_t blenderid;
    name author;

    name collection; // fromtemp and totemp can only be from similar collection
    uint64_t ingredient;
    uint64_t target;

    uint64_t primary_key() const { return blenderid; };
  };

  /*
    BlendConfigs
  */
  TABLE blendconfig_s
  {
    uint64_t blenderid;

    int64_t maxuse = -1;     // -1 = (global use) infinite use
    int64_t maxuseruse = -1; // -1 = (user use) infinite use
    uint64_t uses = 0;

    int64_t startdate = -1; // -1, start as soon
    int64_t enddate = -1;   // -1, does not end
  };

  TABLE config_s
  {
    uint64_t blendercounter = 100000;
  };

  typedef singleton<"config"_n, config_s> config_t;
  typedef multi_index<"config"_n, config_s> config_t_for_abi;
  typedef multi_index<"simblender"_n, simpleblend_s> simblender_t;
  typedef multi_index<"simswap"_n, simpleswap_s> simswap_t;

  /* Initialize tables */
  config_t config = config_t(_self, _self.value);
  simblender_t simblends = simblender_t(_self, _self.value);
  simswap_t simswaps = simswap_t(_self, _self.value);

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

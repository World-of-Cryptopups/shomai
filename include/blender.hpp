#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "atomicassets.hpp"
#include "atomicdata.hpp"

using namespace std;
using namespace eosio;

CONTRACT blender : public contract
{
public:
  using contract::contract;

  /*
    Start Blend Actions
    */
  ACTION makeburner(name user, name targetcol, name targettemp, vector<uint64_t> ingredients);
  ACTION makeswapper(name user, name targetcol, name targettemp, vector<uint64_t> ingredients);
  ACTION delburner(name user, uint64_t burnerid);
  ACTION delswapper(name user, uint64_t swapperid);
  ACTION blendswapper(name blender, vector<uint64_t> assetids);
  ACTION blendburner(name blender, vector<uint64_t> assetids);
  /*
    End Blend Actions
    */

private:
  TABLE burner_s
  {
    uint64_t burnerid;
    name user;
    name targetcol;
    name targettemp;
    vector<uint64_t> ingredients;

    uint64_t primary_key() const { return burnerid; };
    uint64_t by_user() const { return user.value; };
  };

  TABLE swapper_s
  {
    uint64_t swapperid;
    name user;
    name targetcol;
    name targettemp;
    vector<uint64_t> ingredients;

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
  typedef multi_index<"burners"_n, burner_s, indexed_by<"user"_n, const_mem_fun<burner_s, uint64_t, &burner_s::by_user>>> burner_t;
  typedef multi_index<"swappers"_n, swapper_s, indexed_by<"user"_n, const_mem_fun<swapper_s, uint64_t, &swapper_s::by_user>>> swapper_t;

  /* Initialize tables */
  config_t config = config_t(_self, _self.value);
  burner_t burner = burner_t(_self, _self.value);
  swapper_t swapper = swapper_t(_self, _self.value);

  // ======== util functions

  /*
      Check if user is authorized to mint NFTs
   */
  bool isAuthorized(name collection, name user)
  {
    auto itrCollection = atomicassets::collections.require_find(collection.value, "No collection with this name exists!");
    bool authorized = false;
    vector<name> authAccounts = itrCollection->authorized_accounts;
    for (auto it = authAccounts.begin(); it != authAccounts.end() && !authorized; it++)
    {
      if (user == name(*it))
      {
        authorized = true;
      }
    }
    return authorized;
  }
};

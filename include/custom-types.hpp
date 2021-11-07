#include <eosio/eosio.hpp>

using namespace eosio;
using namespace std;

/**
 * Blending config.
 *  - maxuse :: total max use limit
 *  - maxuseruse :: limit for a user to use the blend
 *  - uses :: log of total uses
 * 
 *  - startdate :: when will the blend available
 *  - enddate :: when the blend will end 
*/
struct BlendConfig
{
    int32_t maxuse = -1;     // -1 = (global use) infinite use
    int32_t maxuseruse = -1; // -1 = (user use) infinite use
    uint32_t uses;

    int32_t startdate = -1; // -1, start as soon
    int32_t enddate = -1;   // -1, does not end
};

struct MultiBlendIngredient
{
    name collection;
    uint32_t ingredient;
};

struct SlotBlendIngredientAttributes
{
    string attrib;         // for templates, this should be `any from values` or `single only`
    vector<string> values; // this is blank / if `any from values`, it can be >1 string templates otherwise should only be one
};

/**
 * Ingredient for slot blending.
 *  - collection :: name of collection
 *  - schema :: name of schema
 *  - from :: (templates / immutable_data)
 *  - anyof :: anything from the attributes is allowed for the blend 
 *      otherwise, the nft will require all the attributes (not applicable for templates)
 *  - attributes :: vector<> config or requirements for this slow
 *      if this is empty for templates, it will allow anything from the schema
*/
struct SlotBlendIngredient
{
    name collection;  // name of collection
    name schema;      // schema under collection
    bool schema_only; // anything under this schema
    int8_t from;      // templates || immutable_data (temp|data) 0 == temp  && 1 == data
    bool anyof;       // any from attributes?
    vector<SlotBlendIngredientAttributes> attributes;
};

struct MultiTarget
{
    uint32_t odds;
    uint32_t templateid;
};
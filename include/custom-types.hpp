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
    int64_t maxuse = -1;     // -1 = (global use) infinite use
    int64_t maxuseruse = -1; // -1 = (user use) infinite use
    uint64_t uses;

    int64_t startdate = -1; // -1, start as soon
    int64_t enddate = -1;   // -1, does not end
};

struct MultiBlendIngredient
{
    name collection;
    uint64_t ingredient;
};

struct SlotBlendIngredientAttributes
{
    string attrib;         // for templates, this is the template id
    vector<string> values; // this is blank / empty for templates
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
 *  - display_text :: text to display for the blender to know what to do
*/
struct SlotBlendIngredient
{
    name collection; // name of collection
    name schema;
    string from; // templates || immutable_data
    bool anyof;  // any from attributes?
    vector<SlotBlendIngredientAttributes> attributes;
    string display_text; // text to display instead of auto-generated
};

struct MultiOddTarget
{
    uint32_t odds;
    int32_t templateid;
};
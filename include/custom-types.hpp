#include <eosio/eosio.hpp>

using namespace eosio;
using namespace std;

struct MultiBlendIngredient {
    name collection;
    uint32_t ingredient;
};

struct SlotBlendSchemaIngredient {
    name schema;
};

struct SlotBlendTemplateIngredient {
    vector<uint32_t> templates;
};

struct SlotBlendAttribValuesIngredient {
    string key;
    vector<string> allowed_values;
};

struct SlotBlendAttribIngredient {
    name schema;
    bool require_all_attribs;
    vector<SlotBlendAttribValuesIngredient> attributes;
};

typedef std::variant<SlotBlendSchemaIngredient, SlotBlendTemplateIngredient, SlotBlendAttribIngredient> SlotBlendIngredientProps;

struct SlotBlendIngredient {
    uint8_t type;
    name collection;
    uint32_t amount;
    SlotBlendIngredientProps props;
};

struct MultiTarget {
    uint32_t odds;
    uint32_t templateid;
};
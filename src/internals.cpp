#include <shomaiiblend.hpp>

/**
 * Internal function for checking and validating a template ingredient.
*/
void shomaiiblend::validate_template_ingredient(atomicassets::templates_t _templates, uint64_t assetid)
{
    auto itrIngredient = _templates.find(assetid);

    check(itrIngredient != _templates.end(), "Template ingredient does not exist in collection!");
    check(itrIngredient->transferable, "Template ingredient is not transferable!");
    check(itrIngredient->burnable, "Template ingredient is not burnable!");
}
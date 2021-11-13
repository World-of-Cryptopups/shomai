#include <shomaiiblend.hpp>

/**
 * Internal function for checking and validating a template ingredient.
*/
void shomaiiblend::validate_template_ingredient(atomicassets::templates_t &templates, uint64_t assetid) {
    auto itrIngredient = templates.find(assetid);

    check(itrIngredient != templates.end(), "Template ingredient does not exist in collection!");
    check(itrIngredient->transferable, "Template ingredient is not transferable!");
    check(itrIngredient->burnable, "Template ingredient is not burnable!");
}

/**
 * Internal function to validate the target outcomes.
*/
void shomaiiblend::validate_multitarget(name collection, vector<MultiTarget> targets) {
    uint32_t total_counted_odds = 0;
    uint32_t lastodd = UINT32_MAX;

    auto templates = atomicassets::get_templates(collection);

    for (MultiTarget i : targets) {
        // check first if target template exists or not
        auto itrTemplate = templates.require_find(uint64_t(i.templateid), ("Target template does not exist in collection: " + to_string(i.templateid)).c_str());

        check(i.odds > 0, "Each target outcome must have positive odds.");
        check(i.odds <= lastodd, "The target outcome must be sorted in descending order based on their odds.");
        lastodd = i.odds;

        total_counted_odds += i.odds;
        check(total_counted_odds >= i.odds, "Overflow: Total odds can't be more than 2^32 - 1.");

        // this is a multi target blend, so it should not have a max supply.
        check(itrTemplate->max_supply == 0, "Can only use templates without a max supply");
    }

    // check only if there are more than 1 targets
    if (targets.size() > 1) {
        check(total_counted_odds == TOTALODDS, "Totals odds of target outcomes does not equal the provided total odds.");
    }
}

void shomaiiblend::validate_caller(name user, name collection) {
    require_auth(user);
    blockContract(user);
}
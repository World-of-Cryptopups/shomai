#include <shomaiiblend.hpp>

/**
 * Create a Simple Blend (same collection only)
*/
ACTION shomaiiblend::makeblsimple(name author, name collection, uint64_t target, vector<uint64_t> ingredients)
{
    require_auth(author);
    blockContract(author);

    // validate target collection
    auto itrCol = atomicassets::collections.require_find(collection.value, "This collection does not exist!");

    // validate author if authorized in collection
    check(isAuthorized(collection, author), "You are not authorized in this collection!");

    // validate contract if authorized by collection
    check(isAuthorized(collection, get_self()), "Contract is not authorized in the collection!");

    // get target collection
    auto templates = atomicassets::get_templates(collection);

    // validate template if exists in collection
    check(templates.find(target) != templates.end(), "Template does not exist in collection!");

    // validate ingredient templates
    for (uint64_t i : ingredients)
    {
        validate_template_ingredient(templates, i);
    }

    // get table
    auto _simpleblends = get_simpleblends(collection);

    // get burner counter
    config_s current_config = config.get();
    uint64_t blenderid = current_config.blendercounter++;
    config.set(current_config, get_self());

    // create blend info
    _simpleblends.emplace(author, [&](simpleblend_s &row)
                          {
                              row.blenderid = blenderid;
                              row.author = author;
                              row.collection = collection;
                              row.target = target;
                              row.ingredients = ingredients;
                          });
}

/**
 * Create a Simple Swap. (same collection only)
*/
ACTION shomaiiblend::makeswsimple(name author, name collection, uint64_t target, uint64_t ingredient)
{
    require_auth(author);
    blockContract(author);

    // validate target collection
    auto itrCol = atomicassets::collections.require_find(collection.value, "This collection does not exist!");

    // validate author if authorized in collection
    check(isAuthorized(collection, author), "You are not authorized in this collection!");

    // validate contract if authorized by collection
    check(isAuthorized(collection, get_self()), "Contract is not authorized in the collection!");

    // get target collection
    auto templates = atomicassets::get_templates(collection);

    // validate template if exists in collection
    check(templates.find(target) != templates.end(), "Template does not exist in collection!");

    // validate ingredient template
    validate_template_ingredient(templates, ingredient);

    // get table
    auto _simpleswaps = get_simpleswaps(collection);

    // get burner counter
    config_s current_config = config.get();
    uint64_t blenderid = current_config.blendercounter++;
    config.set(current_config, get_self());

    // create blend info
    _simpleswaps.emplace(author, [&](simpleswap_s &row)
                         {
                             row.blenderid = blenderid;
                             row.author = author;
                             row.collection = collection;
                             row.target = target;
                             row.ingredient = ingredient;
                         });
}

ACTION shomaiiblend::makeblmulti(name author)
{
    require_auth(author);
}

ACTION shomaiiblend::makeblslot(name author, name collection)
{
    require_auth(author);
}
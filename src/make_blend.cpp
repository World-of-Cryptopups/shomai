#include <shomaiiblend.hpp>

/**
 * Create a Simple Blend (same collection only)
*/
ACTION shomaiiblend::makeblsimple(name author, name collection, uint32_t target, vector<uint32_t> ingredients)
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
    for (auto &i : ingredients)
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
ACTION shomaiiblend::makeswsimple(name author, name collection, uint32_t target, uint32_t ingredient)
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

ACTION shomaiiblend::makeblslot(name author, name collection, vector<MultiTarget> targets, vector<SlotBlendIngredient> ingredients, string title)
{
    require_auth(author);
    blockContract(author);

    // validate target collection
    auto itrCol = atomicassets::collections.require_find(collection.value, "This collection does not exist!");

    // validate author if authorized in collection
    check(isAuthorized(collection, author), "You are not authorized in this collection!");

    // validate contract if authorized by collection
    check(isAuthorized(collection, get_self()), "Contract is not authorized in the collection!");

    // check multitarget
    validate_multitarget(collection, targets);

    // validate ingredients
    for (auto &i : ingredients)
    {
        switch (i.index())
        {
        case 0:
        {
            // CHECK SCHEMA SLOT

            auto _schema = get<SlotBlendSchemaIngredient>(i);

            auto itrSchemas = atomicassets::get_schemas(_schema.collection);
            itrSchemas.require_find(_schema.schema.value, "Schema does not exist in this collection!");

            break;
        }
        case 1:
        {
            // CHECK TEMPLATE SLOT

            auto _template = get<SlotBlendTemplateIngredient>(i);

            auto itrTemplates = atomicassets::get_templates(_template.collection);
            for (auto &j : _template.templates)
            {
                itrTemplates.require_find(uint64_t(j), "Template does not exist in this collection!");
            }

            break;
        }
        case 2:
        {
            // CHECK ATTRIBUTE SLOT

            auto _attrib = get<SlotBlendAttribIngredient>(i);

            auto itrSchemas = atomicassets::get_schemas(_attrib.collection);
            auto itr = itrSchemas.require_find(_attrib.schema.value, "Schema does not exist in this collection!");

            for (auto j : _attrib.attributes)
            {
                bool keyExists;

                for (auto s : itr->format)
                {
                    if (s.name == j.key)
                    {
                        keyExists = true;
                    }
                }

                check(keyExists, "Attribute key does not exist in schema!");
            }

            break;
        }
        default:
        {
            check(false, "Invalid ignredient type!");
        }
        }
    }

    // get burner counter
    config_s current_config = config.get();
    uint64_t blenderid = current_config.blendercounter++;
    config.set(current_config, get_self());

    auto slotblends = get_slotblends(collection);
    auto mtargets = get_blendertargets(collection);

    // store slotblend
    slotblends.emplace(author, [&](slotblend_s &row)
                       {
                           row.blenderid = blenderid;
                           row.author = author;

                           row.collection = collection;
                           row.ingredients = ingredients;

                           row.title = title;
                       });

    // store multi targets
    mtargets.emplace(author, [&](multitarget_s &row)
                     {
                         row.blenderid = blenderid;
                         row.collection = collection;

                         row.targets = targets;
                     });
}
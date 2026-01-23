#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/io/json/om_io_token_json.hpp"
#include <cmath>
#include <stack>
#include <stdexcept>

namespace openminecraft::io::json
{
std::shared_ptr<OMJsonNode> OMJsonAstBuilder::build()
{
    std::stack<OMJsonAstContext> contexts;
    std::shared_ptr<OMJsonNode> root;
    auto root_set = false;

    while (!iter->end())
    {
        auto token = iter->next();
        switch (token->type)
        {
        case BeginObject: {
            auto container =
                std::make_shared<OMJsonNodeObject>(std::unordered_map<std::string, std::shared_ptr<OMJsonNode>>());
            if (!root_set)
            {
                root = container;
                root_set = true;
                contexts.push(OMJsonAstContext(container));
            }
            else if (!contexts.empty())
            {
                auto &context = contexts.top();
                if (context.container->type() == Array)
                {
                    context.container->getArray().push_back(container);
                    contexts.push(OMJsonAstContext(container));
                }
                else if (context.waiting_for_value)
                {
                    context.waiting_for_value = false;
                    context.container->getMap()[context.key] = container;
                    contexts.push(OMJsonAstContext(container));
                }
            }
            break;
        }
        case EndObject:
        case EndArray: {
            if (!contexts.empty())
            {
                contexts.pop();
            }
            break;
        }
        case BeginArray: {
            auto container = std::make_shared<OMJsonNodeArray>(std::vector<std::shared_ptr<OMJsonNode>>());
            if (!root_set)
            {
                root = container;
                root_set = true;
                contexts.push(OMJsonAstContext(container));
            }
            else if (!contexts.empty())
            {
                auto &context = contexts.top();
                if (context.container->type() == Array)
                {
                    context.container->getArray().push_back(container);
                    contexts.push(OMJsonAstContext(container));
                }
                else if (context.waiting_for_value)
                {
                    context.waiting_for_value = false;
                    context.container->getMap()[context.key] = container;
                    contexts.push(OMJsonAstContext(container));
                }
            }
            break;
        }
        case StringLiteral: {
            if (contexts.empty())
            {
                throw std::logic_error("unexpected string as json root");
            }
            auto &context = contexts.top();
            if (context.container->type() == Object)
            {
                if (!context.waiting_for_value)
                {
                    context.key = token->content;
                    context.waiting_for_value = true;
                }
                else
                {
                    context.container->getMap()[context.key] = std::make_shared<OMJsonNodeString>(token->content);
                    context.waiting_for_value = false;
                }
            }
            else if (context.container->type() == Array)
            {
                context.container->getArray().push_back(std::make_shared<OMJsonNodeString>(token->content));
            }
            break;
        }

        case NumberLiteral:
        case ConstantLiteral: {
            throw std::logic_error("not implemented");
        }

        case Comma:
        case Colon: {
            break;
        }
        }
    }

    return nullptr;
}
} // namespace openminecraft::io::json

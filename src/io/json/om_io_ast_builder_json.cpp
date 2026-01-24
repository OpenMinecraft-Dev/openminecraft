#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/io/json/om_io_token_json.hpp"
#include <cmath>
#include <fmt/format.h>
#include <stack>
#include <stdexcept>
#include <string>

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
        if (token == nullptr)
        {
            break;
        }
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

        case NumberLiteral: {
            if (contexts.empty())
            {
                throw std::logic_error("unexpected number as json root");
            }
            double num;
            try
            {
                num = std::stod(token->content);
            }
            catch (...)
            {
                throw std::logic_error(fmt::format("unable to parse number: {}", token->content));
            }
            auto &context = contexts.top();
            if (context.container->type() == Object)
            {
                if (!context.waiting_for_value)
                {
                    throw std::logic_error("number cannot be a key");
                }

                context.container->getMap()[context.key] = std::make_shared<OMJsonNodeNumber>(num);
                context.waiting_for_value = false;
            }
            else if (context.container->type() == Array)
            {
                context.container->getArray().push_back(std::make_shared<OMJsonNodeNumber>(num));
            }
            break;
        }
        case ConstantLiteral: {
            if (contexts.empty())
            {
                throw std::logic_error("unexpected constant as json root");
            }
            auto &context = contexts.top();
            std::shared_ptr<OMJsonNode> target;
            if (token->content == "true")
            {
                target = std::make_shared<OMJsonNodePrimitive>(true);
            }
            else if (token->content == "false")
            {
                target = std::make_shared<OMJsonNodePrimitive>(false);
            }
            else if (token->content == "null")
            {
                target = std::make_shared<OMJsonNodeNull>();
            }
            else
            {
                throw std::logic_error(fmt::format("unexpected constant {}", token->content));
            }

            if (context.container->type() == Object)
            {
                if (!context.waiting_for_value)
                {
                    throw std::logic_error("constant cannot be a key");
                }

                context.container->getMap()[context.key] = target;
                context.waiting_for_value = false;
            }
            else if (context.container->type() == Array)
            {
                context.container->getArray().push_back(target);
            }

            break;
        }

        case Comma:
        case Colon: {
            break;
        }
        }
    }

    return root;
}
} // namespace openminecraft::io::json

#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include <functional>
#include <memory>
#include <sstream>

using namespace openminecraft::log;
using namespace openminecraft::io;

int main()
{
    multithread::registerCurrentThreadName("test");
    OMLogger logger("Test");

    logger.info("Hello, world!");

    auto ll = std::make_shared<std::istringstream>("{\"test\": [1, 1e-1, \"test\", {\"33\": true}, false, null]}");

    auto itt = std::make_shared<json::OMJsonTokenIter>(ll);

    json::OMJsonAstBuilder bld(itt);

    std::function<void(std::shared_ptr<json::OMJsonNode>, int depth)> test;
    test = [&](std::shared_ptr<json::OMJsonNode> node, int depth) {
        std::string prex = "";
        for (int i = 0; i < depth * 2; i++)
        {
            prex += " ";
        }
        if (node->type() == json::Object)
        {
            logger.info("{}{{", prex);
            for (auto &pp : node->getMap())
            {
                logger.info("{}\"{}\": ", prex, pp.first);
                test(pp.second, depth + 1);
            }
            logger.info("{}}}", prex);
        }

        if (node->type() == json::Array)
        {
            logger.info("{}[", prex);
            for (auto &p : node->getArray())
            {
                test(p, depth + 1);
            }
            logger.info("{}]", prex);
        }

        if (node->type() == json::Number)
        {
            logger.info("{}{}", prex, node->getNumberFloating());
        }

        if (node->type() == json::String)
        {
            logger.info("{}{}", prex, node->getString());
        }

        if (node->type() == json::Primitive)
        {
            logger.info("{}{}", prex, node->getBoolean() ? "true" : "false");
        }

        if (node->type() == json::Null)
        {
            logger.info("{}null", prex);
        }
    };

    auto t = bld.build();
    test(t, 0);
}

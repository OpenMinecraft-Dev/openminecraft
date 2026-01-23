#include "openminecraft/io/json/om_io_tokeniter_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
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

    json::OMJsonTokenIter itt(ll);

    while (!itt.end())
    {
        auto token = itt.next();
        logger.info("{}", token->content);
    }
}

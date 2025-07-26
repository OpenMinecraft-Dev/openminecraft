#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

namespace openminecraft::vm::pixeltower::v0
{
OMPixelTower::OMPixelTower() : logger("OMPixelTower", this)
{
    heap = new OMPixelTowerHeap(1ul * 1024 * 1024, 1ul * 1024 * 1024 * 1024);
    metaspace = new OMPixelTowerHeap(1ul * 1024 * 1024, 8ul * 1024 * 1024);
    loader = new OMKlassLoader(heap, metaspace);
}
OMPixelTower::~OMPixelTower()
{
    delete loader;
    delete heap;
    delete metaspace;
}

void OMPixelTower::testAppendMethod()
{
    auto alloc = (OMMethod *)metaspace->allocate(sizeof(OMMethod) + 128);
    loader->loadClass("java/lang/Object");
    alloc->klass = loader->fetchClass("java/lang/Object");
    alloc->name = "equals";
    alloc->desc = "(Ljava/lang/Object;)Z";
    alloc->accessFlags = JVM_Acc_Public;
    alloc->codeSize = 128;
    alloc->lineNumberTable = nullptr;

    for (auto i = 0; i < 128; i++)
    {
        alloc->code[i] = 0xcc;
    }
    logger.debug("function inserted at {}", (void *)alloc);

    if (alloc == metaspace->heapBase())
    {
        return;
    }

    auto currentMet = (OMMethod *)metaspace->heapBase();
    while (true)
    {
        if (currentMet->next == nullptr)
        {
            currentMet->next = alloc;
            break;
        }
        else
        {
            currentMet = currentMet->next;
        }
    }
}

void OMPixelTower::init(std::string basePath)
{
    std::vector<std::shared_ptr<std::istream>> target;
    std::filesystem::recursive_directory_iterator itt(basePath);
    for (std::filesystem::recursive_directory_iterator end; end != itt; ++itt)
    {
        if (itt->is_regular_file() && itt->path().extension() == ".class")
        {
            target.push_back(std::make_shared<std::ifstream>(itt->path(), std::ios::binary));
            logger.debug("{} found", itt->path().string());
        }
    }
    init(target);
}

void OMPixelTower::load(std::string path)
{
    std::vector<std::shared_ptr<std::istream>> target;
    target.push_back(std::make_shared<std::ifstream>(path, std::ios::binary));
    init(target);
}

void OMPixelTower::init(std::vector<std::shared_ptr<std::istream>> &streams)
{
    for (auto &s : streams)
    {
        classfile::OMClassFileParser parser(&*s);
        auto target = parser.parse();

        if (target.type == util::Err)
        {
            throw target.unwrap_err();
        }

        loader->stagClass(target.unwrap());
    }
}
} // namespace openminecraft::vm::pixeltower::v0
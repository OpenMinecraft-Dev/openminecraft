#include "openminecraft/vm/impl/om_impl_printstream.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_klass.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_oop.hpp"
#include <cstdint>
#include <iostream>
#include <typeindex>

namespace openminecraft::vm::impl
{
std::any vmstd_internal_SystemPrintStream_println(std::any *args)
{
    auto t = std::type_index(args[1].type());
    if (t.name() == std::type_index(typeid(pixeltower::v0::jlong)).name())
    {
        std::cout << "[stdout] " << std::any_cast<pixeltower::v0::jlong>(args[1]) << std::endl;
    }
    else if (t.name() == std::type_index(typeid(pixeltower::v0::jfloat)).name())
    {
        std::cout << "[stdout] " << std::any_cast<pixeltower::v0::jfloat>(args[1]) << std::endl;
    }
    else if (t.name() == std::type_index(typeid(pixeltower::v0::jboolean)).name())
    {
        std::cout << "[stdout] " << (std::any_cast<pixeltower::v0::jboolean>(args[1]) ? "true" : "false") << std::endl;
    }
    else if (t.name() == std::type_index(typeid(pixeltower::v0::jint)).name())
    {
        std::cout << "[stdout] " << std::any_cast<pixeltower::v0::jint>(args[1]) << std::endl;
    }
    else if (t.name() == std::type_index(typeid(pixeltower::v0::jdouble)).name())
    {
        std::cout << "[stdout] " << std::any_cast<pixeltower::v0::jdouble>(args[1]) << std::endl;
    }
    else if (t.name() == std::type_index(typeid(void *)).name())
    {
        auto t = static_cast<pixeltower::v0::OMOOPDesc *>(std::any_cast<void *>(args[1]));
        if (t->klass->name == "java/lang/String")
        {
            auto h = t->klass->heap;
            void *ptt;
            if (h->ptrCompEnabled())
            {
                ptt = h->decompressPtr(*(uint32_t *)t->data);
            }
            else
            {
                ptt = *(void **)t->data;
            }
            auto arr = static_cast<pixeltower::v0::OMOOPArrDesc *>(ptt);
            std::cout << "[stdout] " << std::string(arr->data, arr->length) << std::endl;
        }
        else
        {
            std::cout << "[stdout] " << t << std::endl;
        }
    }
    return nullptr;
}
} // namespace openminecraft::vm::impl
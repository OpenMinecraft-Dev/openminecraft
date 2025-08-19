#ifndef OM_PIXELTOWER_CLASSBUILDER_HPP
#define OM_PIXELTOWER_CLASSBUILDER_HPP

#include "openminecraft/vm/pixeltower/v0/om_pixeltower_base.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_heap.hpp"
#include "openminecraft/vm/pixeltower/v0/om_pixeltower_method.hpp"
#include <any>
#include <cstdint>
#include <string>
#include <vector>
namespace openminecraft::vm::pixeltower::v3
{
class OMClassBuilder
{
  public:
    OMClassBuilder(v0::OMPixelTowerHeap *heap);
    ~OMClassBuilder() = default;

    void nop();
    void load_constant(std::string def);
    void load_constant(v0::jint def);
    void load_constant(v0::jfloat def);
    void load_constant(v0::jlong def);
    void load_constant(v0::jdouble def);
    void load_constant(v0::jshort def);
    void load_constant(v0::jbyte def);
    void load_constant();
    void constructMethod(std::string name, std::string descriptor);

  private:
    v0::OMPixelTowerHeap *metaspace;
    std::vector<uint8_t> codes;
    std::vector<std::any> consts;
    v0::OMMethod *buildedMethods;
};
} // namespace openminecraft::vm::pixeltower::v3

#endif

#ifndef OM_PIXELTOWER_INTERFACE_HPP
#define OM_PIXELTOWER_INTERFACE_HPP
#include "openminecraft/vm/pixeltower/v0/om_pixeltower.hpp"

namespace openminecraft::vm::pixeltower::v1
{
class OMPixelTowerInterface
{
  public:
    OMPixelTowerInterface(v0::OMPixelTower *);
    ~OMPixelTowerInterface() = default;

    v0::OMField *findField(v0::OMKlass *klass, std::string name, std::string desc);
    template <typename T> void putField(v0::OMOOPDesc *obj, v0::OMField *field, T value)
    {
        if (field && obj)
        {
            if constexpr (std::is_pointer_v<T>)
            {
                if (tower->heap->ptrCompEnabled())
                {
                    *reinterpret_cast<uint32_t *>(&obj->data[field->offset]) = tower->heap->compressPtr(value);
                }
                else
                {
                    *reinterpret_cast<T *>(&obj->data[field->offset]) = value;
                }
            }
            else
            {
                *reinterpret_cast<T *>(&obj->data[field->offset]) = value;
            }
        }
    }
    template <typename T> void putStaticField(v0::OMKlass *klass, v0::OMField *field, T value)
    {
        if (klass && field)
        {
            if constexpr (std::is_pointer_v<T>)
            {
                if (tower->heap->ptrCompEnabled())
                {
                    *static_cast<uint32_t *>(static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)) = tower->heap->compressPtr(value);
                }
                else
                {
                    *static_cast<T *>(static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)) = value;
                }
            }
            else
            {
                *static_cast<T *>(static_cast<void *>(static_cast<uint8_t *>(field->klass->staticBlock) + field->offset)) = value;
            }
        }
    }

  private:
    v0::OMPixelTower *tower;
};
} // namespace openminecraft::vm::pixeltower::v1

#endif
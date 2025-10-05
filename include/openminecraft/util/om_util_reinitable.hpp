#ifndef OM_UTIL_REINITABLE_HPP
#define OM_UTIL_REINITABLE_HPP

namespace openminecraft::util
{
class OMReinitable
{
public:
    virtual void reinit() = 0;
};
}

#endif
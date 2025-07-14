#ifndef OM_PIXELTOWER_HPP
#define OM_PIXELTOWER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/util/om_util_result.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_classloader.hpp"
#include <any>
#include <istream>
#include <memory>
namespace openminecraft::vm::pixeltower
{
class OMPixelTower
{
  public:
    OMPixelTower();
    ~OMPixelTower();

    util::OMResult<std::any, err::OMValidationError> loadClass(std::shared_ptr<classfile::OMClassFile> file);
    util::OMResult<std::any, err::OMValidationError> loadClass(std::shared_ptr<std::istream> file);
    util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError> fetchClass(std::string name);

    util::OMResult<std::any, err::OMValidationError> execute(std::string clazz, std::string name, std::string desc);

    void *allocate(std::shared_ptr<OMClass> cls);

    void debugStackStatus();
    std::string printAny(std::any data);
    std::any interpreter;

  private:
    std::shared_ptr<OMClassLoader> classloader;
    log::OMLogger logger;
};
} // namespace openminecraft::vm::pixeltower

#endif
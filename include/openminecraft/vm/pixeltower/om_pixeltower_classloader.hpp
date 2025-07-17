#ifndef OM_PIXELTOWER_CLASSLOADER_HPP
#define OM_PIXELTOWER_CLASSLOADER_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/vm/classfile/om_class_file.hpp"
#include "openminecraft/vm/pixeltower/om_pixeltower_class_structdef.hpp"
#include <any>
#include <functional>
#include <list>
#include <memory>

namespace openminecraft::vm::pixeltower
{
class OMClassLoader
{
  public:
    OMClassLoader(std::any interpreter);
    ~OMClassLoader();

    util::OMResult<std::shared_ptr<OMClass>, err::OMValidationError> forName(std::string name);
    void appendStagingClass(std::shared_ptr<classfile::OMClassFile> file);
    std::unordered_map<uint64_t, std::shared_ptr<OMClass>> classes;

  protected:
    util::OMResult<std::any, err::OMValidationError> loadClass(std::shared_ptr<classfile::OMClassFile> file);

  private:
    std::list<std::shared_ptr<classfile::OMClassFile>> stagingClasses;
    log::OMLogger logger;
    std::any interpreter;
};
} // namespace openminecraft::vm::pixeltower

#endif
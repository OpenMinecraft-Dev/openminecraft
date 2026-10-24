#ifndef OM_IO_PARSER_HPP
#define OM_IO_PARSER_HPP

#include <iostream>
#include <memory>
#include <utility>

namespace openminecraft::io
{
class OMParser
{
  public:
    // INFO: 只接受 shared_ptr —— 解析器必须持有流对象，否则构造参数里的 shared_ptr
    // 在构造结束时析构，之后的 check()/读取都会访问已释放的流（SIGSEGV 或乱码）。
    explicit OMParser(std::shared_ptr<std::istream> stream);
    ~OMParser();
    auto check() -> bool;

    auto stream() const -> std::istream &
    {
        return *sourceOwner;
    }

  protected:
    std::istream *source;
    std::shared_ptr<std::istream> sourceOwner;
};
} // namespace openminecraft::io

#endif

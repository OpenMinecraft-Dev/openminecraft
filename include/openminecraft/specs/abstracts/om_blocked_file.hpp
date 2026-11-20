#ifndef OM_BLOCKED_FILE_HPP
#define OM_BLOCKED_FILE_HPP

#include <functional>
#include <iostream>
#include <istream>
#include <memory>
#include <unordered_map>
namespace openminecraft::specs
{
using OMBlockHeaderHandler = std::function<void()>;
template <typename T> class OMBlockedFile
{
  public:
    OMBlockedFile() = default;
    ~OMBlockedFile() = default;

    using DataProcessor = std::function<void(std::shared_ptr<std::istream>)>;

    void parse(std::shared_ptr<std::istream> input)
    {
        parseMagic(input);

        bool con = true;
        while (input->good())
        {
            T l;
            con = parseBlockHeader(input, &l);
            if (!con)
            {
                break;
            }
            parseBlockContent(input, l);
        }
    }
    virtual void parseMagic(std::shared_ptr<std::istream>) = 0;
    virtual auto parseBlockHeader(std::shared_ptr<std::istream>, T *) -> bool = 0;
    void parseBlockContent(std::shared_ptr<std::istream> istr, T type)
    {
        for (auto &p : processorMap)
        {
            if (p.first == type)
            {
                p.second(istr);
            }
        }
    }

  protected:
    std::unordered_map<T, DataProcessor> processorMap;
};
}; // namespace openminecraft::specs

#endif

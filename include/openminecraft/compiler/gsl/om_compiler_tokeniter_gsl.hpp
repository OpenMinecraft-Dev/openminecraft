#ifndef OM_COMPILER_TOKENITER_GSL_HPP
#define OM_COMPILER_TOKENITER_GSL_HPP

#include "openminecraft/compiler/gsl/om_compiler_token_gsl.hpp"
#include "openminecraft/compiler/om_compiler_tokeniter.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <optional>

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

namespace openminecraft::compiler::gsl
{
struct OMGslProcessingHook : boost::wave::context_policies::default_preprocessing_hooks
{
  private:
    std::function<std::optional<std::string>(std::string)> fetcher;

  public:
    OMGslProcessingHook(std::function<std::optional<std::string>(std::string)> fetcher) : fetcher(fetcher)
    {
    }

    template <typename ContextT>
    auto locate_include_file(ContextT &ctx, std::string &file_path, bool is_system, char const *current_name,
                             std::string &dir_path, std::string &native_name) -> bool
    {
        if (fetcher(file_path).has_value())
        {
            native_name = file_path;
            dir_path = "";
            return true;
        }

        return false;
    }
};

extern std::vector<std::string> storage;
struct OMGslMemoryInputPolicy
{
  private:
    std::function<std::optional<std::string>(std::string)> fetcher;

  public:
    OMGslMemoryInputPolicy(std::function<std::optional<std::string>(std::string)> fetcher) : fetcher(fetcher)
    {
    }

    template <typename IterContext> class inner
    {
      public:
        template <typename Position>
        void init_iterators(IterContext &iter_ctx, Position const &act_pos, boost::wave::language_support language)
        {
            using iterator_type = typename IterContext::iterator_type;

            const std::string &id = std::string(iter_ctx.filename.c_str());
            auto data = fetcher(id).value_or("");
            storage.emplace_back(data);

            auto &content = storage.back();
            iter_ctx.first =
                typename IterContext::iterator_type(content.begin(), content.end(), Position(id.c_str()), language);
            iter_ctx.last = typename IterContext::iterator_type();
        }
    };
};

class OMGslTokenIter : public OMTokenIter<OMGslToken>
{
  private:
    std::function<std::optional<std::string>(std::string)> fileFetcher;

  public:
    OMGslTokenIter(std::shared_ptr<std::istream> istr,
                   std::function<std::optional<std::string>(std::string)> fileFetcher)
        : OMTokenIter(istr), fileFetcher(std::move(fileFetcher))
    {
    }
    virtual ~OMGslTokenIter() = default;

    auto next() -> std::shared_ptr<OMGslToken> override;
    auto end() -> bool override;
};
} // namespace openminecraft::compiler::gsl

#endif

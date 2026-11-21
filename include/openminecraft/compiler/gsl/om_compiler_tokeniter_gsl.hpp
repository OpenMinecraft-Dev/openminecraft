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
void storeCache(std::string path, std::string content);
auto loadCache(std::string path) -> std::string;
struct OMGslProcessingHook : boost::wave::context_policies::default_preprocessing_hooks
{
  private:
    std::function<std::optional<std::string>(std::string)> fetcher;

  public:
    OMGslProcessingHook(std::function<std::optional<std::string>(std::string)> fetcher) : fetcher(std::move(fetcher))
    {
    }

    template <typename ContextT>
    auto locate_include_file(ContextT &ctx, std::string &file_path, bool is_system, char const *current_name,
                             std::string &dir_path, std::string &native_name) -> bool
    {
        auto istr = fetcher(file_path);
        if (istr.has_value())
        {
            storeCache(file_path, *istr);
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
  public:
    OMGslMemoryInputPolicy() = default;

    template <typename IterContext> class inner
    {
      public:
        template <typename Position>
        void init_iterators(IterContext &iter_ctx, Position const &act_pos, boost::wave::language_support language)
        {
            using iterator_type = typename IterContext::iterator_type;

            const std::string &id = std::string(iter_ctx.filename.c_str());
            auto data = loadCache(id);
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
                   std::function<std::optional<std::string>(std::string)> fileFetcher, std::string name)
        : OMTokenIter(istr), fileFetcher(std::move(fileFetcher))
    {
        std::string input((std::istreambuf_iterator<char>(*istr)), std::istreambuf_iterator<char>());
        ctx =
            std::make_shared<context_type>(input.begin(), input.end(), name.c_str(), OMGslProcessingHook(fileFetcher));
        ctx->set_language(boost::wave::enable_emit_line_directives(ctx->get_language(), false));
        ctx->set_language(boost::wave::enable_va_opt(ctx->get_language(), false));
        ctx->set_language(boost::wave::enable_long_long(ctx->get_language(), false));

        current = ctx->begin();
        final = ctx->end();
    }
    virtual ~OMGslTokenIter() = default;

    auto next() -> std::shared_ptr<OMGslToken> override;
    auto end() -> bool override;

    using token_type = boost::wave::cpplexer::lex_token<>;
    using lex_iterator_type = boost::wave::cpplexer::lex_iterator<token_type>;

    using context_type =
        boost::wave::context<std::string::iterator, lex_iterator_type, OMGslMemoryInputPolicy, OMGslProcessingHook>;

    std::shared_ptr<context_type> ctx;

    context_type::iterator_type current, final;
};
} // namespace openminecraft::compiler::gsl

#endif

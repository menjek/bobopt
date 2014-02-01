#include <bobopt_debug.hpp>
#include <bobopt_utils.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace bobopt
{

    template <typename InputIterator>
    optimizer::optimizer(clang::tooling::Replacements* replacements, InputIterator first, InputIterator last)
        : replacements_(replacements)
    {
        construct(first, last);
    }

    template <typename InputIterator>
    void optimizer::construct(InputIterator first, InputIterator last)
    {
        std::fill_n(std::begin(methods_), methods_.size(), nullptr);
        for (; first != last; ++first)
        {
            create_method(*first);
        }
    }

    BOBOPT_INLINE modes optimizer::get_mode() const
    {
        return mode_;
    }

    BOBOPT_INLINE bool optimizer::verbose() const
    {
        return ((mode_ == MODE_DIAGNOSTIC) || (mode_ == MODE_INTERACTIVE));
    }

    BOBOPT_INLINE diagnostic& optimizer::get_diagnostic()
    {
        BOBOPT_ASSERT(diagnostic_);
        return *diagnostic_;
    }

    BOBOPT_INLINE const diagnostic& optimizer::get_diagnostic() const
    {
        BOBOPT_ASSERT(diagnostic_);
        return *diagnostic_;
    }

    BOBOPT_INLINE void optimizer::enable_method(method_type method)
    {
        create_method(method);
    }

    BOBOPT_INLINE void optimizer::disable_method(method_type method)
    {
        destroy_method(method);
    }

    BOBOPT_INLINE bool optimizer::is_method_enabled(method_type method) const
    {
        BOBOPT_ASSERT(method < OM_COUNT);
        return (methods_[method] != nullptr);
    }

    BOBOPT_INLINE void optimizer::set_compiler(clang::CompilerInstance* compiler)
    {
        BOBOPT_ASSERT(compiler != nullptr);
        compiler_ = compiler;
        diagnostic_ = make_unique<diagnostic>(*compiler_);
    }

    BOBOPT_INLINE clang::CompilerInstance& optimizer::get_compiler() const
    {
        return *compiler_;
    }

} // namespace
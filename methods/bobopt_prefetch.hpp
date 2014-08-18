/// \file bobopt_prefetch.hpp Definition of bobox prefetch optimization method.
///
/// Prefetch optimization method looks at box inputs, analyze box \c init_impl()
/// overriden member function checking for prefetch of inputs, analyze box
/// \c sync_mach_etwas() overriden member function if some inputs need to be
/// prefetched and add their prefetch into \c init_impl() function.
///
/// Expected layout of bobox box class:
/// \code
/// class some_box : public bobox::basic_box {
/// public:
///     BOBOX_BOX_INPUTS_LIST(0, input0, 1, input1)
///
///     //...
///
///     virtual void init_impl() override
///     {
///         //...
///         prefetch_envelope(inputs::input0()); // if input0 is used in body.
///         prefetch_envelope(inputs::input1()); // if input1 is used in body.
///         //...
///     }
///
///     virtual void sync_mach_etwas() override
///     {
///         bobox::input_stream<> input0(this, input_to_inarc(inputs::input0()));
///         bobox::input_stream<> input1(this, input_to_inarc(inputs::input1()));
///
///         //...
///
///         // input0 used here
///         // input1 used here
///     }
/// };
/// \endcode

#ifndef BOBOPT_METHODS_BOBOPT_PREFETCH_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_PREFETCH_HPP_GUARD_

#include <bobopt_language.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_method.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <string>
#include <vector>

// forward declarations:
namespace clang
{
    class CompoundStmt;
    class CXXRecordDecl;
    class CXXMethodDecl;
}

namespace bobopt
{

    namespace methods
    {

        // forward declarations:
        namespace detail
        {
            class prefetch_collector;
            class used_collector;
        }

        /// \brief Main class responsible for prefetch optimization.
        ///
        /// Class is expected to be called from clang tool with pointer to appropriate
        /// replacements.
        ///
        /// \note
        /// Without pointer to replacements, object won't be able to store changes in source files.
        /// The same applies when there are compilation errors. Clang tooling frontend refuse to save changes then.
        /// \endnote
        ///
        /// Method doesn't do \b any optimizations if:
        /// - (global.1) There are no functions.
        ///   Rationale: No input is used in known functions.
        /// - (global.2) There are no inputs.
        ///   Rationale: Nothing to optimize.
        /// - (global.3) Optimizer can't access definition of overriden \c init_impl() function.
        ///   Rationale: There's definition somewhere, but optimizer won't be able to put anything there + ODR.
        /// - (global.4) Corresponding method is not the one from bobox::box and is private.
        ///   Rationale: It's not possible to call such method and that would change code semantic.
        ///
        /// Method doesn't optimize \b single input if:
        /// - (single.1) ...
        class prefetch : public basic_method
        {
        public:

            // create/destroy:
            prefetch();
            virtual ~prefetch() BOBOPT_OVERRIDE;

            // optimization:
            virtual void optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements);

        private:
            BOBOPT_NONCOPYMOVABLE(prefetch);

            // typedefs:
            typedef std::vector<std::string> names_type;
            
            // helpers:
            void prepare();

            void collect_inputs();
            void collect_functions();

            bool analyze_init(detail::prefetch_collector& prefetched);
            void analyze_sync(detail::used_collector& used) const;
            void analyze_body(detail::used_collector& used) const;

            names_type filter_names(const names_type& names, const detail::used_collector& used);
            void insert_into_body(const names_type& to_prefetch, const detail::used_collector& used);
            void insert_init_impl(const names_type& to_prefetch, const detail::used_collector& used);
            void attach_to_body(const names_type& to_prefetch, clang::CompoundStmt* body);

            clang::CXXMethodDecl* get_input(const std::string& name) const;

            void emit_header() const;
            void emit_box_declaration() const;
            void emit_input_declaration(clang::CXXMethodDecl* decl) const;

            // data members:
            clang::CXXRecordDecl* box_;
            clang::tooling::Replacements* replacements_;

            std::vector<clang::CXXMethodDecl*> inputs_;
            clang::CXXMethodDecl* init_;
            clang::CXXMethodDecl* base_init_;
            clang::CXXMethodDecl* sync_;
            clang::CXXMethodDecl* body_;

            std::string decl_indent_;
            std::string line_indent_;
            std::string endl_;

            // constants:
            static const std::string BOX_INIT_FUNCTION_NAME;
            static const std::string BOX_INIT_OVERRIDEN_PARENT_NAME;
            static const std::string BOX_SYNC_FUNCTION_NAME;
            static const std::string BOX_SYNC_OVERRIDEN_PARENT_NAME;
            static const std::string BOX_BODY_FUNCTION_NAME;
            static const std::string BOX_BODY_OVERRIDEN_PARENT_NAME;
        };

    } // namespace methods

    /// \relates method_factory
    /// \brief Function used to create prefetch object.
    basic_method* create_prefetch();

} // namespace bobopt

#endif // guard
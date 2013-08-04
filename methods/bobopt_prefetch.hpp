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
namespace clang {
	class CXXRecordDecl;
	class CXXMethodDecl;
}

namespace bobopt {

	namespace methods {

		// forward declarations:
		namespace internal {
			class prefetched_collector;
			class should_prefetch_collector;
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
		/// - (global.1) There are no inputs.
		///   Rationale: Nothing to optimize.
		/// - (global.2) There's no overriden \c init_impl() function.
		///   Rationale: \c init_impl() is defined elsewhere in inheritance tree and likely can prefetch inputs. Example: bobox::dummy_box base.
		/// - (global.3) Method can't access definition of \c init_impl() overriden function.
		///   Rationale: There's body definition somewhere, but method won't be able to put anything there + ODR.
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
			typedef std::vector<clang::CXXMethodDecl*> inputs_type;
			typedef std::vector<std::string> named_inputs_type;

			typedef clang::CXXMethodDecl* init_function_type;
			typedef clang::CXXMethodDecl* sync_function_type;
			typedef clang::CXXMethodDecl* body_function_type;
			typedef std::vector<clang::CXXMethodDecl*> init_functions_type;
			typedef std::vector<clang::CXXMethodDecl*> body_functions_type;

			// helpers:
			void pre_optimize();
			
			void collect_inputs();
			void collect_functions();

			bool analyze_init(internal::prefetched_collector& prefetched) const;
			bool analyze_sync(internal::should_prefetch_collector& should_prefetch) const;
			bool analyze_body(internal::should_prefetch_collector& should_prefetch) const;
			void add_prefetch(const named_inputs_type& to_prefetch, const internal::should_prefetch_collector& should_prefetch);

			clang::CXXMethodDecl* get_input(const std::string& name) const;

			bool is_verbose() const;
			void emit_header() const;
			void emit_box_declaration() const;
			void emit_input_declaration(clang::CXXMethodDecl* decl) const;

			// data members:
			clang::CXXRecordDecl* box_;
			clang::tooling::Replacements* replacements_;

			inputs_type inputs_;
			init_function_type init_;
			sync_function_type sync_;
			body_function_type body_;

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
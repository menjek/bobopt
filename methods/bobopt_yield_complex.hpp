/// \file bobopt_yield_complex.hpp Definition of optimizing bobox complex member
/// functions execution.
///
/// Complex member functions can hold execution of single bobox pipeline and
/// block execution of other boxes that block execution of other boxes and so
/// on. It basically hold down paralelism.
///
/// Method statically analyzes code of such member function and if it finds it
/// too complex, it statically inserts \c bobox::basic_box::yield() to give up
/// CPU or dynamically injects code that reacts on holding CPU for long time and
/// potentially calls \c bobox::basic_box::yield().

#ifndef BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_

#include <bobopt_method.hpp>
#include <methods/bobopt_complexity_tree.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTTypeTraits.h"
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <memory>
#include <string>
#include <vector>

// forward declarations:
namespace clang {
    class CXXRecordDecl;
	class CXXMethodDecl;
	class CFG;
}

namespace bobopt {

	namespace methods {

		/// \brief Definition of method to optimize complex bobox member functions.
		/// 
		/// Method creates its own AST related tree which holds only nodes that are
		/// potentially going to be expensive and so their execution will be yield.
		/// Such nodes are loops, compound statements, if, call expressions and
		/// try blocks. All rest statements are counted as with complexity is equal
		/// to one.
		///
		/// TODO: description of finding places to insert yield();
		class yield_complex : public basic_method
		{
		public:
		
			// create/destroy:
			yield_complex();
			virtual ~yield_complex() BOBOPT_OVERRIDE;

			// optimize:
			virtual void optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements) BOBOPT_OVERRIDE;

		private:
			BOBOPT_NONCOPYMOVABLE(yield_complex);
			
			// helper structures:

			/// \brief Structure that holds information about member function and name of parent it overrides.
			struct method_override
			{
				std::string method_name;
				std::string parent_name;
			};

			// typedefs:
			typedef clang::CXXMethodDecl* exec_function_type;

			// helpers:
			void optimize_methods();
			void optimize_method(exec_function_type method);
            void optimize_body(const clang::CFG& cfg);

			// data members:
			clang::CXXRecordDecl* box_;
			clang::tooling::Replacements* replacements_;

			// constants.
            static const size_t COMPLEXITY_THRESHOLD = 1500;
			static const size_t BOX_EXEC_METHOD_COUNT = 3;
			static const method_override BOX_EXEC_METHOD_OVERRIDES[BOX_EXEC_METHOD_COUNT];
		};

	} // namespace methods

	/// \relates method_factory
	/// \brief Function used to create yeild_complex object.
	basic_method* create_yield_complex();

} // namespace bobopt

#endif // guard
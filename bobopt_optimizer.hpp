/// \file bobopt_optimizer.hpp Contains base class for bobox optimizations.

#ifndef BOBOPT_OPTIMIZER_HPP_GUARD_
#define BOBOPT_OPTIMIZER_HPP_GUARD_

#include <bobopt_inline.hpp>
#include <bobopt_language.hpp>
#include <bobopt_method.hpp>
#include <bobopt_method_factory.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <array>

// Forward declaration(s).
namespace clang {
	class CXXRecordDecl;
	class CompilerInstance;
}

namespace bobopt {

	/// \brief Optimization level type.
	enum level_type
	{
		OL_NONE = 0,
		OL_BASIC = 1,
		OL_EXTRA = 2,

		OL_COUNT
	};

	/// \brief Base class for bobox optimizations.
	///
	/// Inherited from clang ast match finder callback. It also contains
	/// definition of matcher for finding bobox boxes.
	class optimizer : public clang::ast_matchers::MatchFinder::MatchCallback
	{
	public:

		static const clang::ast_matchers::DeclarationMatcher BOX_MATCHER;

		explicit optimizer(clang::tooling::Replacements* replacements);
		optimizer(clang::tooling::Replacements* replacements, level_type level);

		template<typename InputIterator>
		optimizer(clang::tooling::Replacements* replacements, InputIterator first, InputIterator last);

		~optimizer();

		void set_level(level_type level);
		
		BOBOPT_INLINE void enable_method(method_type method);
		BOBOPT_INLINE void disable_method(method_type method);
		BOBOPT_INLINE bool is_method_enabled(method_type method) const;

		BOBOPT_INLINE clang::CompilerInstance* get_compiler() const;
		BOBOPT_INLINE void set_compiler(clang::CompilerInstance* compiler);

		virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result) BOBOPT_OVERRIDE;

	private:
		typedef const method_type* method_iterator;
		typedef std::pair<method_iterator, method_iterator> method_iterator_pair;

		template<typename InputIterator>
		void construct(InputIterator first, InputIterator last);

		void create_method(method_type method);
		void destroy_method(method_type method);

		void apply_methods(clang::CXXRecordDecl* box_declaration, clang::ASTContext* context) const;
				
		static method_iterator_pair get_level_methods(level_type level);

		clang::CompilerInstance* compiler_;
		clang::tooling::Replacements* replacements_;
		std::array<basic_method*, OM_COUNT> methods_;
	};


} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_optimizer)

#endif // guard
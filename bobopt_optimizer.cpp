#include <bobopt_debug.hpp>
#include <bobopt_method.hpp>
#include <bobopt_method_factory.hpp>
#include <bobopt_optimizer.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceManager.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>

#include BOBOPT_INLINE_IN_SOURCE(bobopt_optimizer)

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace bobopt {

	const DeclarationMatcher optimizer::BOX_MATCHER = recordDecl(isDerivedFrom("bobox::basic_box")).bind("box");

	optimizer::optimizer(modes mode, Replacements* replacements)
		: optimizer(mode, replacements, OL_EXTRA)
	{}

	optimizer::optimizer(modes mode, Replacements* replacements, levels level)
		: mode_(mode)
		, compiler_(nullptr)
		, replacements_(replacements)
	{
		BOBOPT_ASSERT(replacements != nullptr);

		auto level_iterators = get_level_methods(level);
		construct(level_iterators.first, level_iterators.second);
	}

	optimizer::~optimizer()
	{
		for (auto method : methods_)
		{
			delete method;
		}
	}

	void optimizer::set_level(levels level)
	{
		for (auto method : methods_)
		{
			delete method;
		}

		auto level_iterators = get_level_methods(level);
		construct(level_iterators.first, level_iterators.second);
	}

	void optimizer::run(const MatchFinder::MatchResult& result)
	{
		// The rest of clang API uses pointer to non-const :(
		auto box_declaration = const_cast<CXXRecordDecl*>(result.Nodes.getNodeAs<CXXRecordDecl>("box"));
		if (box_declaration != nullptr)
		{
			// Do not optimize boxes in system header files.
			if (result.SourceManager->isInSystemHeader(box_declaration->getLocation()))
			{
				return;
			}

			std::string name = box_declaration->getNameAsString();
			BOBOPT_UNUSED_EXPRESSION(name);

			apply_methods(box_declaration);
		}
	}

	void optimizer::create_method(method_type method)
	{
		BOBOPT_ASSERT(method < OM_COUNT);

		if (methods_[method] != nullptr)
		{
			destroy_method(method);
		}

		methods_[method] = method_factory::create(method);
	}

	void optimizer::destroy_method(method_type method)
	{
		BOBOPT_ASSERT(method < OM_COUNT);

		delete methods_[method];
		methods_[method] = nullptr;
	}

	void optimizer::apply_methods(CXXRecordDecl* box_declaration) const
	{
		BOBOPT_ASSERT(box_declaration != nullptr);

		for (auto method : methods_)
		{
			if (method != nullptr)
			{
				method->set_optimizer(this);
				method->optimize(box_declaration, replacements_);
			}
		}
	}

	optimizer::method_iterator_pair optimizer::get_level_methods(levels level)
	{
		static const method_type METHODS[OM_COUNT] = { OM_PREFETCH, OM_YIELD_COMPLEX };

		switch (level)
		{
			case OL_NONE:
			{
				return std::make_pair(&METHODS[0], &METHODS[0]);
			}

			case OL_BASIC:
			{
				static const size_t METHODS_COUNT = 1;
				return std::make_pair(&METHODS[0], &METHODS[0] + METHODS_COUNT);
			}

			case OL_EXTRA:
			{
				static const size_t METHODS_COUNT = 2;
				return std::make_pair(&METHODS[0], &METHODS[0] + METHODS_COUNT);
			}

			default:
			{
				BOBOPT_ERROR("Should never reach this code.");
			}
		}

		return std::make_pair(nullptr, nullptr);
	}

} // namespace
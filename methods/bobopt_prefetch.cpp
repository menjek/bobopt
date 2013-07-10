#include <bobopt_debug.hpp>
#include <bobopt_language.hpp>
#include <bobopt_macros.hpp>
#include <clang/bobopt_clang_utils.hpp>
#include <clang/bobopt_control_flow_search.hpp>
#include <methods/bobopt_prefetch.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <vector>

BOBOPT_TODO("Maybe own IO management in future.");
#include <iostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::ast_type_traits;
using namespace clang::tooling;
using namespace std;

namespace bobopt {

	namespace methods {
		
		namespace internal {

			// inputs_collector_helper definition.
			//==============================================================================

			/// \relates inputs_collector
			/// \brief Collects all member functions returning input type of bobox boxes.
			///
			/// It expects certain layout of inputs structure. The one created by
			/// \c BOBOX_BOX_INPUT_LIST macro, where the last member function is getter for
			/// input type by its name.
			class inputs_collector_helper BOBOPT_FINAL : public RecursiveASTVisitor<inputs_collector_helper>
			{
			public:

				/// \brief Type of container for holding pointers to declarations of input type function member getters.
				typedef vector<CXXMethodDecl*> inputs_type;

				/// \brief Visit member function and store pointer to this declaration if it's not getter for input by name.
				bool VisitCXXMethodDecl(CXXMethodDecl* decl)
				{
					if (decl->getResultType().getAsString() == INPUTS_RETURN_TYPE_NAME)
					{
						if (decl->getNameAsString() == INPUTS_GETTER_NAME)
						{
							return false;
						}

						inputs_.push_back(decl);
					}
					
					return true;
				}

				/// \brief Access collected inputs.
				BOBOPT_INLINE const inputs_type& get_inputs() const
				{
					return inputs_;
				}

			private:

				// data members:
				inputs_type inputs_;

				// constants:
				static const string INPUTS_RETURN_TYPE_NAME;
				static const string INPUTS_GETTER_NAME;

			};

			// inputs_collector_helper implementation.
			//==============================================================================

			/// \brief Name of bobox box input type.
			const string inputs_collector_helper::INPUTS_RETURN_TYPE_NAME("input_index_type");

			/// \brief Nmae of getter for input by its name.
			const string inputs_collector_helper::INPUTS_GETTER_NAME("get_input_by_name");


			// inputs_collector definition.
			//==============================================================================

			/// \relates inputs_collector_helper
			/// \brief Search in bobox box for \c inputs structure and forward job to helper.
			class inputs_collector BOBOPT_FINAL : public RecursiveASTVisitor<inputs_collector>
			{
			public:

				/// \brief Inherit type for holding inputs from helper.
				typedef inputs_collector_helper::inputs_type inputs_type;

				/// \brief Visit \c struct definition and if its name matches, forward job to helper.
				bool VisitCXXRecordDecl(CXXRecordDecl* decl)
				{
					BOBOPT_ASSERT(decl != nullptr);

					if (decl->getNameAsString() == INPUTS_STRUCT_NAME)
					{
						BOBOPT_CHECK(!collector_helper_.TraverseDecl(decl->getCanonicalDecl()));
						return false;
					}

					return true;
				}

				/// \brief Access collected inputs.
				BOBOPT_INLINE const inputs_type& get_inputs() const
				{
					return collector_helper_.get_inputs();
				}

			private:

				// data member:
				inputs_collector_helper collector_helper_;

				// constants:
				static const string INPUTS_STRUCT_NAME;

			};

			// inputs_collector implementation.
			//==============================================================================

			/// \brief Name of structure that holds bobox box inputs.
			const string inputs_collector::INPUTS_STRUCT_NAME("inputs");


			// extract_type_helper definition.
			//==============================================================================

			/// \brief Helper class to extract input expression from part of AST tree.
			///
			/// It uses \link bobopt::recursive_match_finder recursive_match_finder \endlink
			/// helper to find matches of \c inputs::name() expressions in subtree.
			class extract_input_helper BOBOPT_FINAL
			{
			public:

				/// \brief Container to hold call expressions related to box input.
				typedef vector<CallExpr*> inputs_type;

				/// \brief AST matchers use \c ASTContext class so we need this to be functional.
				explicit extract_input_helper(ASTContext* context)
					: context_(context)
					, callback_()
				{
					BOBOPT_ASSERT(context != nullptr);
				}

				/// \brief Traverse part of AST Tree where root is Stmt node.
				void TraverseStmt(Stmt* stmt)
				{
					MatchFinder finder;
					finder.addMatcher(INPUT_TYPE_MATCHER, &callback_);
					recursive_match_finder recursive_finder(&finder, context_);
					recursive_finder.TraverseStmt(stmt);
				}

				/// \brief Traverse part of AST Tree where root is Decl node.
				void TraverseDecl(Decl* decl)
				{
					MatchFinder finder;
					finder.addMatcher(INPUT_TYPE_MATCHER, &callback_);
					recursive_match_finder recursive_finder(&finder, context_);
					recursive_finder.TraverseDecl(decl);
				}

				/// \brief Traverse part of AST Tree where root is Type node.
				void TraverseType(QualType type)
				{
					MatchFinder finder;
					finder.addMatcher(INPUT_TYPE_MATCHER, &callback_);
					recursive_match_finder recursive_finder(&finder, context_);
					recursive_finder.TraverseType(type);
				}

				/// \brief Access collected results.
				BOBOPT_INLINE const inputs_type& get_inputs() const
				{
					return callback_.get_inputs();
				}
			
			private:

				/// \brief Callback definition for matcher.
				///
				/// We expect to hold only one instance of this matcher callback so
				/// it can hold all values by itself.
				class finder_callback BOBOPT_FINAL : public MatchFinder::MatchCallback
				{
				public:

					/// \brief Default construction.
					finder_callback()
						: inputs_()
					{}

					/// \brief Overriden virtual to handle match result.
					virtual void run(const MatchFinder::MatchResult& result) BOBOPT_OVERRIDE
					{
						CallExpr* call_expr = const_cast<CallExpr*>(result.Nodes.getNodeAs<CallExpr>("callExpr"));
						if (call_expr != nullptr)
						{
							inputs_.push_back(call_expr);
						}
					}

					/// \brief Access to collected inputs.
					BOBOPT_INLINE const inputs_type& get_inputs() const
					{
						return inputs_;
					}

				private:
					BOBOPT_NONCOPYMOVABLE(finder_callback);
					inputs_type inputs_;
				};

				// do not create without AST context:				
				extract_input_helper();

				// data members:
				ASTContext* context_;
				finder_callback callback_;

				// constants:
				static const StatementMatcher INPUT_TYPE_MATCHER;
			};

			// extract_type_helper implementation.
			//==============================================================================

			/// \brief Matcher for calling expressions that returns bobox box input type.
			const StatementMatcher extract_input_helper::INPUT_TYPE_MATCHER = callExpr(hasType(asString("input_index_type"))).bind("callExpr");
			
			
			// prefetched_collector definition.
			//==============================================================================

			/// \relates control_flow_search
			/// \brief Class responsible for handling contain of \c init_impl function.
			///
			/// It looks for \c prefetched_envelope() calls in code that will be surely
			/// visited and collects input names that are prefetched.
			class prefetched_collector BOBOPT_FINAL : public control_flow_search<prefetched_collector, string>
			{
			public:

				/// \brief Type of base class.
				typedef control_flow_search<prefetched_collector, string> base_type;

				/// \relates control_flow_search
				/// \brief Function member required by base class used to prototype current object.
				///
				/// Object doesn't hold any state so prototyped object is just default constructed one.
				prefetched_collector prototype() const
				{
					return prefetched_collector();
				}

				/// \brief Function handles member call expressions.
				///
				/// It should be run on \c init_impl member function in bobox box and \c prefetch_envelope()
				/// is member function of bobox box. If name of member call expr matches, it forwards job to
				/// \link add_prefetched add_prefetched \endlink member function.
				bool VisitCXXMemberCallExpr(CXXMemberCallExpr* expr)
				{
					BOBOPT_ASSERT(expr != nullptr);

					CXXMethodDecl* method = expr->getMethodDecl();
					BOBOPT_ASSERT(method != nullptr);
					if (method->getNameAsString() == PREFETCH_NAME)
					{
						if (expr->getNumArgs() >= 1)
						{
							add_prefetched(expr->getArg(0));
						}
					}

					return true;
				}

			private:

				/// \brief Function handles \c prefetched_envelope member call.
				///
				/// It tries to extract name of input from the first parameter. If it succeeds it stores
				/// input na in class base.
				void add_prefetched(Expr* arg)
				{
					BOBOPT_ASSERT(arg != nullptr);

					if (arg->getType().getAsString() == PREFETCH_ARG_TYPE_NAME)
					{
						string prefetched;
						CallExpr* prefetched_expr;
						if (extract_type(arg, prefetched, prefetched_expr))
						{
							BOBOPT_ASSERT(!prefetched.empty());
							BOBOPT_ASSERT(prefetched_expr != nullptr);
							base_type::insert_value_location(prefetched, DynTypedNode::create(*prefetched_expr));
						}
					}
				}

				/// \brief Accept only \c inputs::name() expressions as \c prefetch_envelope first argument.
				///
				/// Relevant part of AST:
				/// \verbatim
				/// |-CXXConstructExpr 0x4708260 <col:21, col:35> 'input_index_type':'class bobox::generic_distinctizer<struct bobox::input_tag>' 'void (class bobox::generic_distinctizer<struct bobox::input_tag> &&) noexcept' elidable
				/// | `-MaterializeTemporaryExpr 0x4708240 <col:21, col:35> 'class bobox::generic_distinctizer<struct bobox::input_tag>' xvalue
				/// |   `-CallExpr 0x4708160 <col:21, col:35> 'input_index_type':'class bobox::generic_distinctizer<struct bobox::input_tag>'
				/// |     `-ImplicitCastExpr 0x4708148 <col:21, col:29> 'input_index_type (*)(void)' <FunctionToPointerDecay>
				/// |       `-DeclRefExpr 0x4708110 <col:21, col:29> 'input_index_type (void)' lvalue CXXMethod 0x4703770 'right' 'input_index_type (void)'
				/// \endverbatim
				BOBOPT_INLINE static bool extract_type(Expr* arg, string& prefetched, CallExpr*& prefetched_expr)
				{
					// CXXConstructExpr node.
					CXXConstructExpr* construct_expr = llvm::dyn_cast_or_null<CXXConstructExpr>(arg);
					if (construct_expr == nullptr)
					{
						return false;
					}

					if (construct_expr->getNumArgs() == 0)
					{
						return false;
					}

					// MaterializeTemporaryExpr node.
					Expr* mt_arg = construct_expr->getArg(0);
					MaterializeTemporaryExpr* mt_expr = llvm::dyn_cast_or_null<MaterializeTemporaryExpr>(mt_arg);
					if (mt_expr == nullptr)
					{
						return false;
					}

					// CallExpr node.
					CallExpr* call_expr = llvm::dyn_cast_or_null<CallExpr>(mt_expr->GetTemporaryExpr());
					if (call_expr == nullptr)
					{
						return false;
					}

					// Try to get callee node.
					FunctionDecl* direct_callee = call_expr->getDirectCallee();
					if (direct_callee != nullptr)
					{
						prefetched = direct_callee->getNameAsString();
						prefetched_expr = call_expr;
						return true;
					}

					return false;
				}

				// constants:
				static const string PREFETCH_NAME;
				static const string PREFETCH_ARG_TYPE_NAME;
			};

			// prefetched_collector implementation.
			//==============================================================================

			/// \brief Name of bobox box member to prefetch envelope of input.
			const string prefetched_collector::PREFETCH_NAME("prefetch_envelope");

			/// \brief Name of bobox box input type name and argument of prefetch member function.
			const string prefetched_collector::PREFETCH_ARG_TYPE_NAME("input_index_type");


			// should_prefetch_collector definition.
			//==============================================================================

			/// \relates control_flow_search
			/// \brief Class responsible for handling \c sync_mach_etwas member function of
			/// bobox box classes.
			///
			/// It looks for all input streams objects and expects input to be prefetched
			/// if \b any member function is called on input stream object.
			class should_prefetch_collector BOBOPT_FINAL : public control_flow_search<should_prefetch_collector, string>
			{
			public:

				/// \brief Type of class base.
				typedef control_flow_search<should_prefetch_collector, string> base_type;

				/// \brief Type of container for holding values. Inherited from base class.
				typedef base_type::values_type values_type;

				/// \brief Default constructed \b invalid collector.
				should_prefetch_collector()
					: input_streams_()
					, context_(nullptr)
				{}

				/// \brief Collector needs to be created with AST context.
				explicit should_prefetch_collector(ASTContext* context)
					: input_streams_()
					, context_(context)
				{
					BOBOPT_ASSERT(context != nullptr);
				}

				/// \brief Validates collector by setting AST context.
				BOBOPT_INLINE void set_ast_context(ASTContext* context)
				{
					BOBOPT_ASSERT(context != nullptr);
					context_ = context;
				}

				/// \relates control_flow_search
				/// \brief New object should inherited list of defined input streams and associated inputs.
				BOBOPT_INLINE should_prefetch_collector prototype() const
				{
					should_prefetch_collector instance;
					instance.input_streams_ = input_streams_;
					return instance;
				}

				/// \brief Handles all variables declarations since it is node where input stream variable will be declared.
				bool VisitVarDecl(VarDecl* var_decl)
				{
					if (var_decl->getType().getAsString() != INPUT_TYPE_NAME)
					{
						return true;
					}

					if (!var_decl->hasDefinition())
					{
						// We are not able to know what input should be prefetched.
						// Continue.
						return true;
					}

					add_input_stream(var_decl->getDefinition());
					return true;
				}

				/// \brief Handles all member calls on any object in control flow path since it is node where member
				/// function will be called on input stream object.
				bool VisitCXXMemberCallExpr(CXXMemberCallExpr* member_call_expr)
				{
					MemberExpr* callee_expr = llvm::dyn_cast_or_null<MemberExpr>(member_call_expr->getCallee());
					if (callee_expr == nullptr)
					{
						// Something unexpected happened.
						// Continue.
						return true;
					}

					DeclRefExpr* base_expr = llvm::dyn_cast_or_null<DeclRefExpr>(callee_expr->getBase());
					if (base_expr == nullptr)
					{
						// Process only expressions called directly on object, not any other expression.
						// Continue.
						return true;
					}

					VarDecl* var_decl = llvm::dyn_cast_or_null<VarDecl>(base_expr->getDecl());
					if (var_decl == nullptr)
					{
						// Something unexpected happened.
						// Continue.
						return true;
					}

					if (!var_decl->hasDefinition())
					{
						// Process only defined variables.
						// Continue.
						return true;
					}

					prefetch_input_stream(var_decl->getDefinition(), member_call_expr);
					return true;
				}

			private:
				
				/// \brief Insert input stream into container based on its definition.
				void add_input_stream(VarDecl* var_def)
				{
					BOBOPT_ASSERT(var_def != nullptr);
					Expr* init_expr = var_def->getInit();
					if (init_expr != nullptr)
					{
						extract_input_helper helper(context_);
						helper.TraverseStmt(init_expr);

						const extract_input_helper::inputs_type& inputs = helper.get_inputs();
						if (inputs.size() == 1)
						{
							input_streams_.insert(make_pair(var_def, inputs.front()));
						}
					}
				}

				/// \brief Insert prefetched input into base class values based on variable definition.
				void prefetch_input_stream(VarDecl* var_def, CXXMemberCallExpr* member_call_expr)
				{
					BOBOPT_ASSERT(var_def != nullptr);

					auto found = input_streams_.find(var_def);
					if (found != end(input_streams_))
					{
						CallExpr* input_call_expr = found->second;
						insert_value_location(input_call_expr->getDirectCallee()->getNameAsString(), DynTypedNode::create(*member_call_expr));
					}
				}

				// data members:
				map<VarDecl*, CallExpr*> input_streams_;
				ASTContext* context_;

				// constants:
				static const string INPUT_TYPE_NAME;
			};

			// should_prefetch_collector implementation.
			//==============================================================================

			/// \brief Name of input stream variable type.
			const string should_prefetch_collector::INPUT_TYPE_NAME("bobox::input_stream<>");
			
		} // namespace internal


		// prefetch implementation.
		//==============================================================================

		/// \brief Construct default empty invalid prefetch object.
		prefetch::prefetch()
			: box_(nullptr)
			, replacements_(nullptr)
			, inputs_()
			, init_(nullptr)
			, body_(nullptr)
		{}

		/// \brief Deletable through pointer to base.
		prefetch::~prefetch()
		{}

		/// \brief Main optimization function.
		///
		/// Step by step prepares object for optimization pass, collect inputs,
		/// collect related functions, collect inputs to prefetch, collect already
		/// prefetched inputs and pass values to function that handles adding prefetch
		/// calls to \c init_impl() member function.
		///
		/// Process to be efficient can decide at almost any point that optimization
		/// can't be done and exit.
		///
		/// \param box AST node representsing bobox box class.
		/// \param replacements Clang replacements structure to edit source files.
		void prefetch::optimize(CXXRecordDecl* box, Replacements* replacements)
		{
			BOBOPT_ASSERT(box != nullptr);
			BOBOPT_ASSERT(replacements != nullptr);

			pre_optimize();

			box_ = box;
			replacements_ = replacements;

			collect_inputs();

			// (global.1) There are no inputs. Don't optimize.
			if (!inputs_.empty())
			{
				collect_functions();

				internal::should_prefetch_collector should_prefetch(basic_method::get_ast_context());
				if (!analyze_body_method(should_prefetch, body_))
				{
					// Don't optimize.
					return;
				}

				named_inputs_type should_prefetch_names = should_prefetch.get_values();
				if (should_prefetch_names.empty())
				{
					// Nothing to optimize.
					return;
				}

				internal::prefetched_collector prefetched;
				if (!analyze_init_method(prefetched, init_))
				{
					// Don't optimize.
					return;
				}

				named_inputs_type prefetched_names = prefetched.get_values();

				sort(begin(prefetched_names), end(prefetched_names));
				sort(begin(should_prefetch_names), end(should_prefetch_names));

				named_inputs_type to_prefetch_names;
				to_prefetch_names.reserve(should_prefetch_names.size());

				set_difference(
					begin(should_prefetch_names), end(should_prefetch_names), // A
					begin(prefetched_names), end(prefetched_names), // B
					back_inserter(to_prefetch_names) // A - B
				);

				if (!to_prefetch_names.empty())
				{
					add_prefetch(to_prefetch_names, should_prefetch);
				}
			}
		}

		/// \brief Prepare object to optimization.
		void prefetch::pre_optimize()
		{
			inputs_.clear();
			init_ = nullptr;
			body_ = nullptr;

			init_functions_.clear();
			init_functions_.resize(BOX_INIT_FUNCTIONS_COUNT, nullptr);
			body_functions_.clear();
			body_functions_.resize(BOX_BODY_FUNCTIONS_COUNT, nullptr);
		}

		/// \brief Collect declarations of inputs from box definition.
		void prefetch::collect_inputs()
		{
			BOBOPT_ASSERT(box_ != nullptr);

			internal::inputs_collector collector;
			bool found = !collector.TraverseDecl(box_->getCanonicalDecl());
	
			if (found)
			{
				BOBOPT_ASSERT(!collector.get_inputs().empty());
				inputs_ = collector.get_inputs();
			}
		}

		/// \brief Collect \c init_impl() and \c sync_mach_etwas() function declarations from box definition.
		void prefetch::collect_functions()
		{
			BOBOPT_ASSERT(box_ != nullptr);

			for (auto method_it = box_->method_begin(); method_it != box_->method_end(); ++method_it)
			{
				CXXMethodDecl* method = *method_it;

				for (size_t i = 0; i < BOX_INIT_FUNCTIONS_COUNT; ++i)
				{
					const auto& method_info = BOX_INIT_FUNCTIONS[i];
					if ((method->getNameAsString() == method_info.name) && overrides(method, method_info.top_overriden_parent))
					{
						BOBOPT_ASSERT(i < init_functions_.size());
						BOBOPT_ASSERT(init_functions_[i] == nullptr);
						init_functions_[i] = method;
					}
				}

				for (size_t i = 0; i < BOX_BODY_FUNCTIONS_COUNT; ++i)
				{
					const auto& method_info = BOX_BODY_FUNCTIONS[i];
					if ((method->getNameAsString() == method_info.name) && overrides(method, method_info.top_overriden_parent))
					{
						BOBOPT_ASSERT(i < body_functions_.size());
						BOBOPT_ASSERT(body_functions_[i] == nullptr);
						body_functions_[i] = method;
					}
				}

				if (method_it->getNameAsString() == BOX_INIT_FUNCTION_NAME)
				{
					if (overrides(method, string("bobox::box")))
					{
						BOBOPT_ASSERT(init_ == nullptr);
						init_ = method;
					}

					continue;
				}

				if (method_it->getNameAsString() == BOX_BODY_FUNCTION_NAME)
				{
					if (overrides(method, string("bobox::basic_box")))
					{
						BOBOPT_ASSERT(body_ == nullptr);
						body_ = method;
					}
				}
			}
		}

		/// \brief Analyze all overriden init virtual functions.
		bool prefetch::analyze_init(internal::prefetched_collector& prefetched) const
		{
			for (auto init_decl : init_functions_)
			{
				if (init_decl != nullptr)
				{
					analyze_init_method(prefetched, init_decl);
				}
			}

			return true;
		}

		/// \brief Analyze \c init_impl() member function.
		///
		/// \param prefetched Reference to internal \link intenral::prefetched_collector prefetched_collector \endlink object.
		/// \return Returns whether optimization process should continue.
		bool prefetch::analyze_init_method(internal::prefetched_collector& prefetched, clang::CXXMethodDecl* init_decl) const
		{
			if (init_decl == nullptr)
			{
				// (global.2) There's no overriden init_impl() function.
				// Do not optimize.
				return false;
			}

			if (!init_decl->hasBody())
			{
				// (global.3) Method can't access definition of init_impl() overriden function.
				// Do not optimize.
				return false;
			}

			if (prefetched.TraverseStmt(init_decl->getBody()))
			{
				// Finished everything OK.
				// Optimize.
				return true;
			}

			// Unexpected traversal ending.
			// Do not optimize.
			return false;
		}

		/// \brief Analyze \c sync_mach_etwas() member function.
		///
		/// \param should_prefetch Reference to internal \link intenral::should_prefetch_collector should_prefetch_collector \endlink object.
		/// \return Returns whether optimization process should continue.
		bool prefetch::analyze_body_method(internal::should_prefetch_collector& should_prefetch, clang::CXXMethodDecl* body_decl) const
		{
			if (body_decl == nullptr)
			{
				// (global.4) There's no overriden sync_mach_etwas() function.
				// Do not optimize.
				return false;
			}

			if (!body_decl->hasBody())
			{
				// (global.5) Method can't access definition of sync_mach_etwas() overriden function.
				// Do not optimize.
				return false;
			}
			
			if (should_prefetch.TraverseStmt(body_decl->getBody()))
			{
				// Finished everything OK.
				// Optimize.
				return true;
			}

			// Unexpected traversal ending.
			// Do not optimize.
			return false;
		}

		/// \brief Insert prefetch calls to \c init_impl() overriden member function.
		///
		/// \param to_prefetch Names of inputs to be prefetched.
		/// \param should_prefetch Holder of source locations for reasoning why inputs should be prefetched.
		void prefetch::add_prefetch(const named_inputs_type& to_prefetch, const internal::should_prefetch_collector& should_prefetch)
		{
			BOBOPT_ASSERT(init_ != nullptr);
			BOBOPT_ASSERT(init_->hasBody());
			BOBOPT_ASSERT(init_->getBody() != nullptr);

			CompoundStmt* body = llvm::dyn_cast_or_null<CompoundStmt>(init_->getBody());
			if (body != nullptr)
			{
				ASTContext* context = basic_method::get_ast_context();
				Rewriter rewriter(context->getSourceManager(), context->getLangOpts());

				cout << "[prefetch] optimization of: " << box_->getNameAsString() << endl;

				string box_location_text = box_->getLocation().printToString(context->getSourceManager());
				cout << "declared here: " << box_location_text << endl;

				for (auto named_input : to_prefetch)
				{
					CXXMethodDecl* input_decl = get_input(named_input);
					if (input_decl == nullptr)
					{
						// We don't have input with this name.
						continue;
					}

					cout << "Missing input: '" << named_input << "'" << endl;
					
// 					string prefetch_text;
// 
// 					prefetch_text += "prefetch_envelope(inputs::";
// 					prefetch_text += named_input;
// 					prefetch_text += "()); ";
// 
// 					rewriter.InsertTextAfterToken(body->getLBracLoc(), prefetch_text.c_str());
				}
			
//				flush_rewriter(rewriter);
			}
		}
		
		/// \brief Tests whether member function overrides bobox basic box virtual member function.
		bool prefetch::overrides(CXXMethodDecl* method_decl, const string& parent_name) const
		{
			for (auto it = method_decl->begin_overridden_methods(); it != method_decl->end_overridden_methods(); ++it)
			{
				const CXXMethodDecl* overridden = *it;
				if (overridden->getParent()->getQualifiedNameAsString() == parent_name)
				{
					return true;
				}
			}

			return false;
		}

		/// \brief Access input member function declaration to access input by name.
		CXXMethodDecl* prefetch::get_input(const string& name) const
		{
			for (auto input : inputs_)
			{
				if (input->getNameAsString() == name)
				{
					return input;
				}
			}

			return nullptr;
		}

		/// \brief Hardcoded constant array of bobox box initialization virtual member functions
		/// together with fully qualified names of topmost overriden parents.
		const prefetch::bobox_method_info prefetch::BOX_INIT_FUNCTIONS[BOX_INIT_FUNCTIONS_COUNT] =
		{
			{ "init_impl", "bobox::box" }
		};

		/// \brief Hardcoded constant array of bobox box body virtual member functions
		/// together with fully qualified names of topmost overriden parents.
		const prefetch::bobox_method_info prefetch::BOX_BODY_FUNCTIONS[BOX_BODY_FUNCTIONS_COUNT] =
		{
			{ "sync_body", "bobox::basic_box" },
			{ "sync_mach_etwas", "bobox::basic_box" },
			{ "async_mach_etwas", "bobox::basic_box" }
		};

		/// \brief Name of bobox box initialization virtual member function to be overriden. 
		const string prefetch::BOX_INIT_FUNCTION_NAME("init_impl");

		/// \brief Name of bobox box body virtual member function to be overriden.
		const string prefetch::BOX_BODY_FUNCTION_NAME("sync_mach_etwas");
	
	} // namespace methods

	basic_method* create_prefetch()
	{
		return new methods::prefetch;
	}

} // namespace bobopt
#include <methods/bobopt_prefetch.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_language.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_optimizer.hpp>
#include <clang/bobopt_clang_utils.hpp>
#include <clang/bobopt_control_flow_search.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

BOBOPT_TODO("Couldn't find anything in llvm code base for input stream :(");
#include <iostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::ast_type_traits;
using namespace clang::tooling;

namespace bobopt
{

    namespace methods
    {
        // Constants.
        //======================================================================

        /// \brief Name of bobox box initialization virtual member function to be overriden.
        const std::string prefetch::BOX_INIT_FUNCTION_NAME("init_impl");
        /// \brief Name of parent overriden functions for init. Just to check if it is not overloaded virtual.
        const std::string prefetch::BOX_INIT_OVERRIDEN_PARENT_NAME("bobox::box");

        /// \brief Name of bobox box sync virtual member function to be overriden.
        const std::string prefetch::BOX_SYNC_FUNCTION_NAME("sync_mach_etwas");
        /// \brief Name of parent overriden functions for sync. Just to check if it is not overloaded virtual.
        const std::string prefetch::BOX_SYNC_OVERRIDEN_PARENT_NAME("bobox::basic_box");

        /// \brief Name of bobox box body virtual member function to be overriden.
        const std::string prefetch::BOX_BODY_FUNCTION_NAME("sync_body");
        /// \brief Name of parent overriden functions for body. Just to check if it is not overloaded virtual.
        const std::string prefetch::BOX_BODY_OVERRIDEN_PARENT_NAME("bobox:basic_box");

        namespace detail
        {

            // inputs_collector_helper definition.
            //==================================================================

            /// \relates inputs_collector
            /// \brief Collects all member functions returning input type of
            /// bobox boxes.
            ///
            /// It expects certain layout of inputs structure. The one created
            /// by \c BOBOX_BOX_INPUT_LIST macro, where the last member function
            /// is getter for input type by its name.
            class inputs_collector_helper : public RecursiveASTVisitor<inputs_collector_helper>
            {
            public:

                /// \brief Type of container for holding pointers to declarations of input type function member getters.
                typedef std::vector<CXXMethodDecl*> inputs_type;

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
                inputs_type inputs_;

                static const std::string INPUTS_RETURN_TYPE_NAME;
                static const std::string INPUTS_GETTER_NAME;
            };

            // inputs_collector_helper implementation.
            //==================================================================

            /// \brief Name of bobox box input type.
            const std::string inputs_collector_helper::INPUTS_RETURN_TYPE_NAME("input_index_type");

            /// \brief Nmae of getter for input by its name.
            const std::string inputs_collector_helper::INPUTS_GETTER_NAME("get_input_by_name");

            // inputs_collector definition.
            //==================================================================

            /// \relates inputs_collector_helper
            /// \brief Search in bobox box for \c inputs structure and forward job to helper.
            class inputs_collector : public RecursiveASTVisitor<inputs_collector>
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
                inputs_collector_helper collector_helper_;

                static const std::string INPUTS_STRUCT_NAME;
            };

            // inputs_collector implementation.
            //==================================================================

            /// \brief Name of structure that holds bobox box inputs.
            const std::string inputs_collector::INPUTS_STRUCT_NAME("inputs");


            // init_collector definition.
            //==================================================================

            /// \relates control_flow_search
            /// \brief Class responsible for handling contain of \c init_impl function.
            ///
            /// It looks for \c prefetched_envelope() calls in code that will be surely
            /// visited and collects input names that are prefetched.
            class init_collector : public control_flow_search<init_collector, std::string>
            {
            public:

                /// \brief Type of base class.
                typedef control_flow_search<init_collector, std::string> base_type;

                /// \relates control_flow_search
                /// \brief Function member required by base class used to prototype current object.
                ///
                /// Object doesn't hold any state so prototyped object is just default constructed one.
                init_collector prototype() const
                {
                    return init_collector();
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
                        std::string prefetched;
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
                /// |-CXXConstructExpr 0x4708260 <col:21, col:35> 'input_index_type':'class bobox::generic_distinctizer<struct bobox::input_tag>'
                /// 'void (class bobox::generic_distinctizer<struct bobox::input_tag> &&) noexcept' elidable
                /// | `-MaterializeTemporaryExpr 0x4708240 <col:21, col:35> 'class bobox::generic_distinctizer<struct bobox::input_tag>' xvalue
                /// |   `-CallExpr 0x4708160 <col:21, col:35> 'input_index_type':'class bobox::generic_distinctizer<struct bobox::input_tag>'
                /// |     `-ImplicitCastExpr 0x4708148 <col:21, col:29> 'input_index_type (*)(void)' <FunctionToPointerDecay>
                /// |       `-DeclRefExpr 0x4708110 <col:21, col:29> 'input_index_type (void)' lvalue CXXMethod 0x4703770 'right' 'input_index_type
                /// (void)'
                /// \endverbatim
                BOBOPT_INLINE static bool extract_type(Expr* arg, std::string& prefetched, CallExpr*& prefetched_expr)
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

                    FunctionDecl* direct_callee = call_expr->getDirectCallee();
                    if (direct_callee != nullptr)
                    {
                        prefetched = direct_callee->getNameAsString();
                        prefetched_expr = call_expr;
                        return true;
                    }

                    return false;
                }

                static const std::string PREFETCH_NAME;
                static const std::string PREFETCH_ARG_TYPE_NAME;
            };

            // init_collector implementation.
            //==================================================================

            /// \brief Name of bobox box member to prefetch envelope of input.
            const std::string init_collector::PREFETCH_NAME("prefetch_envelope");

            /// \brief Name of bobox box input type name and argument of prefetch member function.
            const std::string init_collector::PREFETCH_ARG_TYPE_NAME("input_index_type");


            // body_collector definition.
            //==================================================================

            /// \relates control_flow_search
            /// \brief Class responsible for handling \c sync_mach_etwas member function of
            /// bobox box classes.
            ///
            /// It looks for all input streams objects and expects input to be prefetched
            /// if \b any member function is called on input stream object.
            class body_collector : public control_flow_search<body_collector, std::string>
            {
            public:

                /// \brief Type of class base.
                typedef control_flow_search<body_collector, std::string> base_type;

                /// \brief Type of container for holding values. Inherited from base class.
                typedef base_type::values_type values_type;

                /// \brief Default constructed \b invalid collector.
                body_collector()
                    : input_streams_()
                    , context_(nullptr)
                {
                }

                /// \brief Collector needs to be created with AST context.
                explicit body_collector(ASTContext* context)
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
                BOBOPT_INLINE body_collector prototype() const
                {
                    body_collector instance;
                    instance.input_streams_ = input_streams_;
                    return instance;
                }

                /// \brief Looking up bobox::input_stream<> variables definitions.
                bool VisitVarDecl(VarDecl* var_decl)
                {
                    if (var_decl->getType().getAsString() != INPUT_STREAM_TYPE_NAME)
                    {
                        return true;
                    }

                    if (!var_decl->hasDefinition())
                    {
                        return true;
                    }

                    add_input_stream(var_decl->getDefinition());
                    return true;
                }

                /// \brief Looking up member calls of bobox::input_stream<> variables.
                bool VisitCXXMemberCallExpr(CXXMemberCallExpr* member_call_expr)
                {
                    MemberExpr* callee_expr = llvm::dyn_cast_or_null<MemberExpr>(member_call_expr->getCallee());
                    if (callee_expr == nullptr)
                    {
                        return true;
                    }

                    DeclRefExpr* base_expr = llvm::dyn_cast_or_null<DeclRefExpr>(callee_expr->getBase());
                    if (base_expr == nullptr)
                    {
                        return true;
                    }

                    VarDecl* var_decl = llvm::dyn_cast_or_null<VarDecl>(base_expr->getDecl());
                    if (var_decl == nullptr)
                    {
                        return true;
                    }

                    if (!var_decl->hasDefinition())
                    {
                        return true;
                    }

                    prefetch_input_stream(var_decl->getDefinition(), member_call_expr);
                    return true;
                }

            private:

                /// \brief Extract stream name from definition of bobox::input_stream<> variable.
                void add_input_stream(VarDecl* var_decl)
                {
                    BOBOPT_ASSERT(var_decl != nullptr);

                    Expr* init_expr = var_decl->getInit();
                    if (init_expr == nullptr)
                    {
                        return;
                    }

                    struct finder_callback : public MatchFinder::MatchCallback
                    {
                        virtual void run(const MatchFinder::MatchResult& result) BOBOPT_OVERRIDE
                        {
                            CallExpr* call_expr = const_cast<CallExpr*>(result.Nodes.getNodeAs<CallExpr>("call_expr"));
                            if (call_expr != nullptr)
                            {
                                inputs.push_back(call_expr);
                            }
                        }

                        std::vector<CallExpr*> inputs;
                    };

                    MatchFinder finder;
                    finder_callback callback;
                    finder.addMatcher(INPUT_INDEX_TYPE_CALL_MATCHER, &callback);
                    recursive_match_finder recursive_finder(&finder, context_);
                    recursive_finder.TraverseStmt(init_expr);

                    // Ignore either ambigous or none.
                    if (callback.inputs.size() == 1)
                    {
                        input_streams_.insert(std::make_pair(var_decl, callback.inputs.front()));
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
                
                /// \brief Declaration of bobox::input_stream<> variable and call to inputs::name() functions.
                std::map<VarDecl*, CallExpr*> input_streams_;
                ASTContext* context_;

                static const std::string INPUT_STREAM_TYPE_NAME;
                static const StatementMatcher INPUT_INDEX_TYPE_CALL_MATCHER;
            };

            // body_collector implementation.
            //==================================================================

            /// \brief Name of input stream variable type.
            const std::string body_collector::INPUT_STREAM_TYPE_NAME("bobox::input_stream<>");

            /// \brief Matcher for call to static inputs::name() function.
            const StatementMatcher body_collector::INPUT_INDEX_TYPE_CALL_MATCHER = callExpr(hasType(asString("input_index_type"))).bind("call_expr");

        } // namespace detail


        // prefetch implementation.
        //======================================================================

        /// \brief Construct default empty invalid prefetch object.
        prefetch::prefetch()
            : box_(nullptr)
            , replacements_(nullptr)
            , inputs_()
            , init_(nullptr)
            , body_(nullptr)
        {
        }

        /// \brief Deletable through pointer to base.
        prefetch::~prefetch()
        {
        }

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

            prepare();

            box_ = box;
            replacements_ = replacements;

            collect_inputs();

            // (global.1) There are no inputs. Don't optimize.
            if (!inputs_.empty())
            {
                collect_functions();

                detail::init_collector prefetched;
                if (!analyze_init(prefetched))
                {
                    return;
                }

                detail::body_collector should_prefetch(&(basic_method::get_optimizer().get_compiler().getASTContext()));
                if (!analyze_sync(should_prefetch))
                {
                    return;
                }

                // Do not analyze body if there's sync.
                if ((sync_ == nullptr) && !analyze_body(should_prefetch))
                {
                    return;
                }

                named_inputs_type should_prefetch_names = should_prefetch.get_values();
                if (should_prefetch_names.empty())
                {                  
                    return;
                }

                named_inputs_type prefetched_names = prefetched.get_values();

                std::sort(std::begin(prefetched_names), std::end(prefetched_names));
                std::sort(std::begin(should_prefetch_names), std::end(should_prefetch_names));

                named_inputs_type to_prefetch_names;
                to_prefetch_names.reserve(should_prefetch_names.size());

                std::set_difference(std::begin(should_prefetch_names),
                                    std::end(should_prefetch_names), // A
                                    std::begin(prefetched_names),
                                    std::end(prefetched_names),           // B
                                    std::back_inserter(to_prefetch_names) // A - B
                                    );

                if (!to_prefetch_names.empty())
                {
                    insert_prefetch(to_prefetch_names, should_prefetch);
                }
            }
        }

        /// \brief Prepare object to optimization.
        void prefetch::prepare()
        {
            inputs_.clear();
            init_ = nullptr;
            sync_ = nullptr;
            body_ = nullptr;
        }

        /// \brief Collect declarations of inputs from box definition.
        void prefetch::collect_inputs()
        {
            BOBOPT_ASSERT(box_ != nullptr);

            detail::inputs_collector collector;
            bool found = !collector.TraverseDecl(box_->getCanonicalDecl());

            if (found)
            {
                BOBOPT_ASSERT(!collector.get_inputs().empty());
                inputs_ = collector.get_inputs();
            }
        }

        /// \brief Collect \c init_impl() and \c sync_mach_etwas() and \c sync_body() function sdeclarations from box definition.
        void prefetch::collect_functions()
        {
            BOBOPT_ASSERT(box_ != nullptr);

            for (auto method_it = box_->method_begin(); method_it != box_->method_end(); ++method_it)
            {
                CXXMethodDecl* method = *method_it;

                if (method->getNameAsString() == BOX_INIT_FUNCTION_NAME)
                {
                    if (overrides(method, BOX_INIT_OVERRIDEN_PARENT_NAME))
                    {
                        BOBOPT_ASSERT(init_ == nullptr);
                        init_ = method;
                    }

                    continue;
                }

                if (method->getNameAsString() == BOX_SYNC_FUNCTION_NAME)
                {
                    if (overrides(method, BOX_SYNC_OVERRIDEN_PARENT_NAME))
                    {
                        BOBOPT_ASSERT(sync_ == nullptr);
                        sync_ = method;
                    }

                    continue;
                }

                if (method->getNameAsString() == BOX_BODY_FUNCTION_NAME)
                {
                    if (overrides(method, BOX_BODY_OVERRIDEN_PARENT_NAME))
                    {
                        BOBOPT_ASSERT(body_ == nullptr);
                        body_ = method;
                    }
                }
            }
        }

        /// \brief Analyze \c init_impl() member function.
        ///
        /// \param prefetched Reference to detail \link intenral::init_collector init_collector \endlink object.
        /// \return Returns whether optimization process should continue.
        bool prefetch::analyze_init(detail::init_collector& prefetched) const
        {
            if (init_ == nullptr)
            {
                // (global.2) There's no overriden init_impl() function.
                return false;
            }

            if (!init_->hasBody())
            {
                // (global.3) Method can't access definition of init_impl() overriden function.
                return false;
            }

            prefetched.TraverseStmt(init_->getBody());
            return true;
        }

        /// \brief Analyze \c sync_mach_etwas() member function.
        ///
        /// \param should_prefetch Reference to detail \link detail::body_collector body_collector \endlink object.
        /// \return Returns whether optimization process should continue.
        bool prefetch::analyze_sync(detail::body_collector& should_prefetch) const
        {
            if (!sync_->hasBody())
            {
                return false;
            }

            should_prefetch.TraverseStmt(sync_->getBody());
            return true;
        }

        /// \brief Analyze \c sync_body() member function.
        ///
        /// \param should_prefetch Reference to detail \link intenral::body_collector body_collector \endlink object.
        /// \return Returns whether optimization process should continue.
        bool prefetch::analyze_body(detail::body_collector& should_prefetch) const
        {
            if (body_ == nullptr)
            {
                return false;
            }

            if (!body_->hasBody())
            {
                return false;
            }

            should_prefetch.TraverseStmt(body_->getBody());
            return true;
        }

        /// \brief Insert prefetch calls to \c init_impl() overriden member function.
        ///
        /// \param to_prefetch Names of inputs to be prefetched.
        /// \param should_prefetch Holder of source locations for reasoning why inputs should be prefetched.
        void prefetch::insert_prefetch(const named_inputs_type& to_prefetch, const detail::body_collector& should_prefetch)
        {
            BOBOPT_ASSERT(init_ != nullptr);
            BOBOPT_ASSERT(init_->hasBody());
            BOBOPT_ASSERT(init_->getBody() != nullptr);

            CompoundStmt* body = llvm::dyn_cast_or_null<CompoundStmt>(init_->getBody());
            if (body == nullptr)
            {
                return;
            }

            if (get_optimizer().verbose())
            {
                emit_header();
                emit_box_declaration();
                llvm::outs() << "\n\n";
            }

            const diagnostic& diag = basic_method::get_optimizer().get_diagnostic();
            for (auto named_input : to_prefetch)
            {
                CXXMethodDecl* input_decl = get_input(named_input);

                if (input_decl == nullptr)
                {
                    continue;
                }

                std::string prefetch_call_source = "prefetch_envelope(inputs::" + named_input + "())";

                bool update_init_impl = false;
                if (get_optimizer().verbose())
                {
                    emit_input_declaration(input_decl);

                    auto locations = should_prefetch.get_locations(named_input);
                    for (auto location : locations)
                    {
                        const CallExpr* call_expr = location.get<CallExpr>();
                        if (call_expr != nullptr)
                        {
                            source_message use_message = diag.get_message_call_expr(source_message::info, call_expr, "used here:");
                            diag.emit(use_message);
                        }
                    }
                    llvm::outs() << '\n';

                    std::string update_message_text = "box initialization phase should call " + prefetch_call_source;
                    source_message update_message = diag.get_message_decl(source_message::suggestion, init_, update_message_text);
                    diag.emit(update_message);

                    if (get_optimizer().get_mode() == MODE_INTERACTIVE)
                    {
                        char answer = 0;
                        while ((answer != 'y') && (answer != 'n'))
                        {
                            llvm::outs() << "Do you wish to update source [y/n]?: ";
                            llvm::outs().flush();
                            std::cin >> answer;
                            answer = static_cast<char>(tolower(answer));
                        }

                        update_init_impl = (answer == 'y');
                        llvm::outs() << "\n\n";
                    }
                }

                if (update_init_impl || (get_optimizer().get_mode() == MODE_BUILD))
                {
                    CompoundStmt* body = llvm::dyn_cast_or_null<CompoundStmt>(init_->getBody());
                    BOBOPT_ASSERT(body != nullptr);

                    SourceLocation insert_location = Lexer::getLocForEndOfToken(
                        body->getLBracLoc(), 0, get_optimizer().get_compiler().getSourceManager(), get_optimizer().get_compiler().getLangOpts());

                    Replacement replacement(get_optimizer().get_compiler().getSourceManager(), insert_location, 0, prefetch_call_source + "; ");

                    replacements_->insert(replacement);

                    if (get_optimizer().verbose())
                    {
                        source_message opt_message = diag.get_message_decl(source_message::optimization, init_, prefetch_call_source + " added.");
                        diag.emit(opt_message);
                        llvm::outs() << "\n\n";
                    }
                }
            }
        }

        /// \brief Access input member function declaration to access input by name.
        CXXMethodDecl* prefetch::get_input(const std::string& name) const
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

        /// \brief Emit header of box optimization.
        void prefetch::emit_header() const
        {
            llvm::raw_ostream& out = llvm::outs();

            out.changeColor(llvm::raw_ostream::WHITE, true);
            out << "[prefetch]";
            out.resetColor();
            out << " optimization of box ";
            out.changeColor(llvm::raw_ostream::MAGENTA, true);
            out << box_->getNameAsString();
            out.resetColor();
            out << '\n';
        }

        /// \brief Emit info about box declaration.
        void prefetch::emit_box_declaration() const
        {
            const diagnostic& diag = basic_method::get_optimizer().get_diagnostic();

            source_message box_message = diag.get_message_decl(source_message::info, box_, "declared here:");
            diag.emit(box_message);
        }

        /// \brief Emit info about input declaration.
        void prefetch::emit_input_declaration(CXXMethodDecl* decl) const
        {
            const diagnostic& diag = basic_method::get_optimizer().get_diagnostic();

            source_message input_message = diag.get_message_decl(source_message::info, decl, "missing prefetch for input declared here:");
            diag.emit(input_message);
        }

    } // namespace methods

    basic_method* create_prefetch()
    {
        return new methods::prefetch;
    }

} // namespace bobopt
#include <methods/bobopt_prefetch.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_language.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_optimizer.hpp>
#include <bobopt_text_utils.hpp>
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

                /// \brief Construct using AST context.
                explicit init_collector(ASTContext* context = nullptr) : base_type(context)
                {
                }

                /// \relates control_flow_search
                /// \brief Function member required by base class used to prototype current object.
                ///
                /// Object doesn't hold any state so prototyped object is just default constructed one.
                init_collector prototype() const
                {
                    return init_collector(base_type::context_);
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

                /// \brief Collector needs to be created with AST context.
                explicit body_collector(ASTContext* context = nullptr, std::map<VarDecl*, CallExpr*> input = std::map<VarDecl*, CallExpr*>())
                    : base_type(context)
                    , input_streams_(input)
                {
                }

                /// \relates control_flow_search
                /// \brief New object should inherite list of defined input streams and associated inputs.
                BOBOPT_INLINE body_collector prototype() const
                {
                    return body_collector(base_type::context_, input_streams_);
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
                    recursive_match_finder recursive_finder(&finder, base_type::context_);
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
            , base_init_(nullptr)
            , sync_(nullptr)
            , body_(nullptr)
            , decl_indent_()
            , line_indent_()
            , endl_()
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

            collect_functions();

            if ((body_ == nullptr) && (sync_ == nullptr))
            {
                // (global.1) There are no functions.
                return;
            }

            collect_inputs();

            if (inputs_.empty())
            {
                // (global.2) There are no inputs.
                return;
            }

            auto& context = get_optimizer().get_compiler().getASTContext();
            detail::init_collector prefetched(&context);
            if (!analyze_init(prefetched))
            {
                return;
            }

            detail::body_collector used(&context);
            analyze_sync(used);
            analyze_body(used);

            names_type used_names = used.get_values();
            if (used_names.empty())
            {
                return;
            }

            names_type prefetched_names = prefetched.get_values();

            std::sort(std::begin(prefetched_names), std::end(prefetched_names));
            std::sort(std::begin(used_names), std::end(used_names));

            names_type to_prefetch_names;
            to_prefetch_names.reserve(used_names.size());

            std::set_difference(std::begin(used_names),
                                std::end(used_names), // A
                                std::begin(prefetched_names),
                                std::end(prefetched_names),           // B
                                std::back_inserter(to_prefetch_names) // A - B
                                );

            if (!to_prefetch_names.empty())
            {
                if (init_ != nullptr)
                {
                    insert_into_body(to_prefetch_names, used);
                }
                else
                {
                    insert_init_impl(to_prefetch_names, used);
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
            detail::inputs_collector collector;
            bool found = !collector.TraverseDecl(box_->getCanonicalDecl());

            if (found)
            {
                inputs_ = collector.get_inputs();
                BOBOPT_ASSERT(!inputs_.empty());
            }
        }

        /// \brief Collect \c init_impl() and \c sync_mach_etwas() and \c sync_body() function sdeclarations from box definition.
        void prefetch::collect_functions()
        {
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
        /// \param prefetched Reference to detail \link detail::init_collector init_collector \endlink object.
        /// \return Returns whether optimization process should continue.
        bool prefetch::analyze_init(detail::init_collector& prefetched)
        {
            if (init_ == nullptr)
            {
                CXXRecordDecl* bobox_box = get_optimizer().get_bobox_box();
                auto found_it = std::find_if(bobox_box->method_begin(), bobox_box->method_end(), [](const CXXMethodDecl * method) {
                    return method->getNameAsString() == "init_impl";
                });
                BOBOPT_ASSERT(found_it != bobox_box->method_end());

                base_init_ = (*found_it)->getCorrespondingMethodInClass(box_);
                BOBOPT_ASSERT(base_init_ != nullptr);

                // (global.4) Corresponding method is not the one from bobox::box and is private.
                return ((base_init_->getParent() == bobox_box) || (base_init_->getAccess() != AS_private));
            }

            if (!init_->hasBody())
            {
                // (global.3) Method can't access definition of init_impl().
                return false;
            }

            prefetched.TraverseStmt(init_->getBody());
            return true;
        }

        /// \brief Analyze \c sync_mach_etwas() member function.
        ///
        /// \param used Reference to detail \link detail::body_collector body_collector \endlink object.
        /// \return Returns whether optimization process should continue.
        void prefetch::analyze_sync(detail::body_collector& used) const
        {
            if (sync_ == nullptr)
            {
                return;
            }

            if (!sync_->hasBody())
            {
                return;
            }

            used.TraverseStmt(sync_->getBody());
        }

        /// \brief Analyze \c sync_body() member function.
        ///
        /// \param used Reference to detail \link intenral::body_collector body_collector \endlink object.
        /// \return Returns whether optimization process should continue.
        void prefetch::analyze_body(detail::body_collector& used) const
        {
            if (body_ == nullptr)
            {
                return;
            }

            if (!body_->hasBody())
            {
                return;
            }

            used.TraverseStmt(body_->getBody());
        }

        /// \brief Insert prefetch calls to overriden \c init_impl() member function.
        ///
        /// \param to_prefetch Names of inputs to be prefetched.
        /// \param should_prefetch Holder of source locations for reasoning why inputs should be prefetched.
        void prefetch::insert_into_body(const names_type& to_prefetch, const detail::body_collector& used)
        {
            BOBOPT_ASSERT(init_ != nullptr);
            BOBOPT_ASSERT(init_->hasBody());

            CompoundStmt* body = llvm::dyn_cast_or_null<CompoundStmt>(init_->getBody());
            BOBOPT_ASSERT(body != nullptr);

            std::string body_indent;
            SourceManager& sm = get_optimizer().get_compiler().getSourceManager();
            if (body->body_empty())
            {
                line_indent_ = detect_line_indent(sm, box_);
                body_indent = decl_indent(sm, init_) + line_indent_;
            }
            else
            {
                body_indent = stmt_indent(sm, body->body_back());
            }

            const bool verbose = get_optimizer().verbose();
            if (verbose)
            {
                emit_header();
                emit_box_declaration();
            }

            bool update_source = false;

            endl_ = detect_line_end(sm, box_);

            std::string prefetch_source;
            const diagnostic& diag = basic_method::get_optimizer().get_diagnostic();
            for (auto named_input : to_prefetch)
            {
                CXXMethodDecl* input_decl = get_input(named_input);
                BOBOPT_ASSERT(input_decl != nullptr);

                const std::string prefetch_call_source = "prefetch_envelope(inputs::" + named_input + "());";
                if (verbose)
                {
                    emit_input_declaration(input_decl);

                    auto locations = used.get_locations(named_input);
                    for (auto location : locations)
                    {
                        const CallExpr* call_expr = location.get<CallExpr>();
                        BOBOPT_ASSERT(call_expr != nullptr);

                        diag.emit(diag.get_message_stmt(diagnostic_message::info, call_expr, "used here:"));
                    }
                    llvm::outs() << endl_;

                    diag.emit(diag.get_message_decl(diagnostic_message::suggestion, init_, "prefetch input in init:"));

                    if (get_optimizer().get_mode() == MODE_INTERACTIVE)
                    {
                        if (ask_yesno("Do you wish to add call?"))
                        {
                            update_source = true;
                            prefetch_source += endl_ + body_indent + prefetch_call_source;
                        }

                        llvm::outs() << endl_ << endl_;
                    }
                }
                else
                {
                    prefetch_source += endl_ + body_indent + prefetch_source;
                }
            }

            if (update_source || (get_optimizer().get_mode() == MODE_BUILD))
            {
                if (!body->body_empty())
                {
                    prefetch_source += endl_;
                }

                SourceLocation location = Lexer::getLocForEndOfToken(body->getLBracLoc(), 0, sm, get_optimizer().get_compiler().getLangOpts());
                replacements_->insert(Replacement(sm, location, 0, prefetch_source));
            }
        }

        /// \brief Create overriden \c init_impl() implementation, calling base and prefetching input.
        void prefetch::insert_init_impl(const names_type& to_prefetch, const detail::body_collector& used)
        {
            auto& sm = get_optimizer().get_compiler().getSourceManager();

            const std::string declaration = "virtual void init_impl()";
            line_indent_ = detect_line_indent(sm, box_);
            decl_indent_ = detect_method_decl_indent(sm, box_);
            endl_ = detect_line_end(sm, box_);

            const bool verbose = get_optimizer().verbose();

            if (verbose)
            {
                emit_header();
                emit_box_declaration();
            }

            const std::string box_indent_ = decl_indent(sm, box_);
            const std::string body_indent = decl_indent_ + line_indent_;
            std::string implementation = box_indent_ + "protected:" + endl_ + decl_indent_ + declaration + endl_ + decl_indent_ + '{' + endl_;

            BOBOPT_ASSERT(base_init_ != nullptr);
            if (base_init_->getParent() != get_optimizer().get_bobox_box())
            {
                implementation += body_indent + base_init_->getParent()->getQualifiedNameAsString() + "::init_impl();" + endl_;
            }

            bool update_source = false;

            const diagnostic& diag = basic_method::get_optimizer().get_diagnostic();
            for (const auto& named_input : to_prefetch)
            {
                CXXMethodDecl* input_decl = get_input(named_input);
                BOBOPT_ASSERT(input_decl != nullptr);

                const std::string prefetch_call_source = "prefetch_envelope(inputs::" + named_input + "());";
                if (verbose)
                {
                    emit_input_declaration(input_decl);

                    auto locations = used.get_locations(named_input);
                    for (auto location : locations)
                    {
                        const CallExpr* call_expr = location.get<CallExpr>();
                        BOBOPT_ASSERT(call_expr != nullptr);

                        diag.emit(diag.get_message_stmt(diagnostic_message::info, call_expr, "used here:"));
                    }
                    llvm::outs() << endl_;

                    if (get_optimizer().get_mode() == MODE_INTERACTIVE)
                    {
                        if (ask_yesno("Do you wish to add call to newly created init_impl()?"))
                        {
                            update_source = true;
                            implementation += body_indent + prefetch_call_source + endl_;
                        }
                    }
                }
                else
                {
                    implementation += body_indent + prefetch_call_source + endl_;
                }
            }

            if (update_source || (get_optimizer().get_mode() == MODE_BUILD))
            {
                implementation += decl_indent_ + "}" + endl_;

                SourceLocation location = box_->getRBraceLoc();
                replacements_->insert(Replacement(sm, location, 0, implementation));
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

            diagnostic_message box_message = diag.get_message_decl(diagnostic_message::info, box_, "declared here:");
            diag.emit(box_message);

            llvm::outs() << '\n';
        }

        /// \brief Emit info about input declaration.
        void prefetch::emit_input_declaration(CXXMethodDecl* decl) const
        {
            const diagnostic& diag = basic_method::get_optimizer().get_diagnostic();

            diagnostic_message input_message = diag.get_message_decl(diagnostic_message::info, decl, "missing prefetch for input declared here:");
            diag.emit(input_message);
        }

    } // namespace methods

    basic_method* create_prefetch()
    {
        return new methods::prefetch;
    }

} // namespace bobopt
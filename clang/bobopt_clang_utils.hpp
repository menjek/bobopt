/// \file bobopt_clang_utils.hpp Contains definitions of various utils extending clang functionality.

#ifndef BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_
#define BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_language.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "llvm/Support/type_traits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceLocation.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <iterator>
#include <string>
#include <vector>

namespace clang
{
    namespace ast_matchers
    {
        class MatchFinder;
    }

    class ASTContext;
    class Decl;
    class Stmt;
    class Type;
    class Rewriter;
    class CXXMethodDecl;
}

namespace bobopt
{

    // global functions.
    //==========================================================================

    /// \brief Tests whether member function overrides bobox basic box virtual member function.
    bool overrides(const clang::CXXMethodDecl* method_decl, const std::string& parent_name);

    /// \brief Receive indent of AST node/location.
    std::string location_indent(const clang::SourceManager& sm, clang::SourceLocation location);
    std::string decl_indent(const clang::SourceManager& sm, const clang::Decl* decl);
    std::string stmt_indent(const clang::SourceManager& sm, const clang::Stmt* stmt);


    // ast_match_finder definition.
    //==========================================================================

    /// \brief Handles AST traversal and match finding.
    ///
    /// By clang API, matchers can be used only once on whole translation unit
    /// and user can use matcher callback to refactor code. Sometimes user want
    /// to look for matching nodes in subtree of AST.
    ///
    /// Clang AST matchers have \c match() member function that does what are
    /// matchers supposed to do, but for \b single node. Class combines them
    /// with RecursiveASTVisitor so client can match nodes in whole subtree.
    ///
    /// \code
    /// MatchFinder finder;
    /// my_callback callback;
    /// finder.addMatcher(recordDecl(hasName("X")), &callback);
    /// recursive_match_finder recursive_finder(&finder, get_ast_context());
    /// finder.TraverseStmt(subtree);
    /// \endcode
    class recursive_match_finder : public clang::RecursiveASTVisitor<recursive_match_finder>
    {
    public:
        // create:
        recursive_match_finder(clang::ast_matchers::MatchFinder* match_finder, clang::ASTContext* context);

        // visit of basic node hierarchies:
        bool VisitDecl(clang::Decl* decl);
        bool VisitStmt(clang::Stmt* stmt);
        bool VisitType(clang::Type* type);

    private:
        clang::ast_matchers::MatchFinder* match_finder_;
        clang::ASTContext* context_;
    };


    namespace detail
    {
        // basic_ast_node_collector definition.
        //======================================================================

        /// \brief Helper for as \c ast_nodes collector<NodeT>. Holder of collected nodes.
        template <typename NodeT>
        class basic_ast_nodes_collector
        {
        public:
            typedef NodeT node_type;
            typedef std::vector<NodeT*> nodes_type;
            typedef typename nodes_type::const_iterator nodes_iterator;
            typedef typename nodes_type::size_type size_type;

            nodes_iterator nodes_begin() const;
            nodes_iterator nodes_end() const;

            bool empty() const;
            size_type size() const;

            node_type* operator[](size_type index) const;

        protected:
            basic_ast_nodes_collector();
            ~basic_ast_nodes_collector();

            nodes_type nodes_;
        };

        template <typename NodeT>
        basic_ast_nodes_collector<NodeT>::basic_ast_nodes_collector()
        {
        }

        template <typename NodeT>
        basic_ast_nodes_collector<NodeT>::~basic_ast_nodes_collector()
        {
        }

        template <typename NodeT>
        typename basic_ast_nodes_collector<NodeT>::nodes_iterator basic_ast_nodes_collector<NodeT>::nodes_begin() const
        {
            return std::begin(nodes_);
        }

        template <typename NodeT>
        typename basic_ast_nodes_collector<NodeT>::nodes_iterator basic_ast_nodes_collector<NodeT>::nodes_end() const
        {
            return std::end(nodes_);
        }

        template <typename NodeT>
        bool basic_ast_nodes_collector<NodeT>::empty() const
        {
            return nodes_.empty();
        }

        template <typename NodeT>
        typename basic_ast_nodes_collector<NodeT>::size_type basic_ast_nodes_collector<NodeT>::size() const
        {
            return nodes_.size();
        }
        template <typename NodeT>
        NodeT* basic_ast_nodes_collector<NodeT>::operator[](size_type index) const
        {
            return nodes_[index];
        }

    } // namespace detail


    // ast_nodes_collector definition.
    //==========================================================================

    /// \brief Collect specific node classes in AST subtree.
    ///
    /// Example:
    /// \code
    /// ast_nodes_collector<CallExpr> collector;
    /// collector.TraverseStmt(stmt);
    /// for (const auto* call_expr : collector)
    /// {
    ///     ...
    /// }
    /// \endcode
    template <typename NodeT, typename Distinctizer = void>
    class ast_nodes_collector;

    template <typename NodeT>
    class ast_nodes_collector<NodeT,
                              typename llvm::enable_if<llvm::is_base_of<clang::Decl, NodeT> >::
                                  type> : public detail::basic_ast_nodes_collector<NodeT>,
                                          public clang::RecursiveASTVisitor<
                                              ast_nodes_collector<NodeT, typename llvm::enable_if<llvm::is_base_of<clang::Decl, NodeT> >::type> >
    {
    public:
        typedef detail::basic_ast_nodes_collector<NodeT> base;

        bool VisitDecl(clang::Decl* decl)
        {
            BOBOPT_ASSERT(decl != nullptr);

            NodeT* node = llvm::dyn_cast<NodeT>(decl);
            if (node != nullptr)
            {
                this->nodes_.push_back(node);
            }
            return true;
        }
    };

    template <typename NodeT>
    class ast_nodes_collector<NodeT,
                              typename llvm::enable_if<llvm::is_base_of<clang::Stmt, NodeT> >::
                                  type> : public detail::basic_ast_nodes_collector<NodeT>,
                                          public clang::RecursiveASTVisitor<
                                              ast_nodes_collector<NodeT, typename llvm::enable_if<llvm::is_base_of<clang::Stmt, NodeT> >::type> >
    {
    public:
        typedef detail::basic_ast_nodes_collector<NodeT> base;

        bool VisitStmt(clang::Stmt* stmt)
        {
            BOBOPT_ASSERT(stmt != nullptr);

            NodeT* node = llvm::dyn_cast<NodeT>(stmt);
            if (node != nullptr)
            {
                this->nodes_.push_back(node);
            }
            return true;
        }
    };

    template <typename NodeT>
    class ast_nodes_collector<NodeT,
                              typename llvm::enable_if<llvm::is_base_of<clang::Type, NodeT> >::
                                  type> : public detail::basic_ast_nodes_collector<NodeT>,
                                          public clang::RecursiveASTVisitor<
                                              ast_nodes_collector<NodeT, typename llvm::enable_if<llvm::is_base_of<clang::Type, NodeT> >::type> >
    {
    public:
        typedef detail::basic_ast_nodes_collector<NodeT> base;

        bool VisitType(clang::Type* type)
        {
            BOBOPT_ASSERT(type != nullptr);
            
            NodeT* node = llvm::dyn_cast<NodeT>(stmt);
            if (node != nullptr)
            {
                this->nodes_.push_back(node);
            }
            return true;
        }
    };

    template<typename NodeT, typename Distinctizer>
    auto begin(const ast_nodes_collector<NodeT, Distinctizer>& collector) -> decltype(collector.nodes_begin())
    {
        return collector.nodes_begin();
    }

    template<typename NodeT, typename Distinctizer>
    auto end(const ast_nodes_collector<NodeT, Distinctizer>& collector) -> decltype(collector.nodes_end())
    {
        return collector.nodes_end();
    }

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_clang_utils)

#endif // guard
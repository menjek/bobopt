/// \file bobopt_clang_utils.hpp Contains definitions of various utils extending
/// clang functionality.

#ifndef BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_
#define BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_

#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "llvm/Support/type_traits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/bobopt_clang_epilog.hpp>

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

    /// \brief Tests whether member function overrides virtual member function
    /// of parent class with specific name.
    ///
    /// \param method_decl Member function tested for override.
    /// \param parent_name Fully qualified name of base class.
    bool overrides(const clang::CXXMethodDecl* method_decl, const std::string& parent_name);

    // ast_match_finder definition.
    //==========================================================================

    /// \brief Class handles AST traversal and match finding.
    ///
    /// Clang API defines matchers so they can be used only once on whole
    /// translation unit and user can use matcher callback and provided
    /// \c Replacements to refactor the code.
    ///
    /// Clang AST matchers have \c match() member function that does what are
    /// matchers supposed to do, but for single node. Class combines matchers
    /// with \c clang::RecursiveASTVisitor so client can match nodes in specific
    /// subtree.
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
        recursive_match_finder(clang::ast_matchers::MatchFinder* match_finder, clang::ASTContext* context);

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

        /// \brief Base for \c nodes_collector<NodeT>.
        ///
        /// Class is base for all \c nodes_collector<NodeT> template
        /// specialization. It holds collected nodes and provides interface
        /// to access them.
        ///
        /// \tparam NodeT Type of the Clang AST node, e.g.,
        /// \code clang::CXXRecordDecl.
        template <typename NodeT>
        class basic_nodes_collector
        {
            typedef std::vector<NodeT*> nodes_type;

        public:
            typedef NodeT node_type;
            typedef typename nodes_type::const_iterator nodes_iterator;
            typedef typename nodes_type::size_type size_type;

            nodes_iterator nodes_begin() const;
            nodes_iterator nodes_end() const;

            bool empty() const;
            size_type size() const;

            node_type* operator[](size_type index) const;

        protected:
            ~basic_nodes_collector();

            nodes_type nodes_;
        };

        // basic_ast_node_collector implementation.
        //======================================================================

        /// \brief Protection for delete through pointer to base.
        template <typename NodeT>
        basic_nodes_collector<NodeT>::~basic_nodes_collector()
        {
        }

        /// \brief Constant iterator to the first element.
        template <typename NodeT>
        typename basic_nodes_collector<NodeT>::nodes_iterator basic_nodes_collector<NodeT>::nodes_begin() const
        {
            return std::begin(nodes_);
        }

        /// \brief Constant iterator to the first element after the last one.
        template <typename NodeT>
        typename basic_nodes_collector<NodeT>::nodes_iterator basic_nodes_collector<NodeT>::nodes_end() const
        {
            return std::end(nodes_);
        }

        /// \brief Detects whether there's any node collected.
        template <typename NodeT>
        bool basic_nodes_collector<NodeT>::empty() const
        {
            return nodes_.empty();
        }

        /// \brief Returns number of nodes stored inside.
        template <typename NodeT>
        typename basic_nodes_collector<NodeT>::size_type basic_nodes_collector<NodeT>::size() const
        {
            return nodes_.size();
        }

        /// \brief \c std::vector like access to collected nodes.
        ///
        /// Function behaves exactly the same as \c std::vector::operator[],
        /// i.e., access out of bounds is UB.
        template <typename NodeT>
        NodeT* basic_nodes_collector<NodeT>::operator[](size_type index) const
        {
            return nodes_[index];
        }

    } // namespace detail

    // nodes_collector definition.
    //==========================================================================

    /// \brief Collect specific node classes in AST subtree.
    ///
    /// \tparam NodeT Type of the Clang AST node, e.g., \c clang::CXXRecordDecl.
    /// \tparam DistinctizerT Type to distinguish basic AST node hierarchies.
    ///
    /// Example:
    /// \code
    /// nodes_collector<CallExpr> collector;
    /// collector.TraverseStmt(stmt);
    /// for (const auto* call_expr : collector)
    /// {
    ///     ...
    /// }
    /// \endcode
    template <typename NodeT, typename DistinctizerT = void>
    class nodes_collector;

    /// \brief Specialization for \c clang::Decl nodes.
    template <typename NodeT>
    class nodes_collector<
        NodeT,
        typename llvm::enable_if<llvm::is_base_of<clang::Decl, NodeT> >::
            type> : public detail::basic_nodes_collector<NodeT>,
                    public clang::RecursiveASTVisitor<nodes_collector<NodeT, typename llvm::enable_if<llvm::is_base_of<clang::Decl, NodeT> >::type> >
    {
    public:
        typedef detail::basic_nodes_collector<NodeT> base_class;

        bool VisitDecl(clang::Decl* decl)
        {
            NodeT* node = llvm::dyn_cast<NodeT>(decl);
            if (node != nullptr)
            {
                this->nodes_.push_back(node);
            }
            return true;
        }
    };

    /// \brief Specialization for \c clang::Stmt nodes.
    template <typename NodeT>
    class nodes_collector<
        NodeT,
        typename llvm::enable_if<llvm::is_base_of<clang::Stmt, NodeT> >::
            type> : public detail::basic_nodes_collector<NodeT>,
                    public clang::RecursiveASTVisitor<nodes_collector<NodeT, typename llvm::enable_if<llvm::is_base_of<clang::Stmt, NodeT> >::type> >
    {
    public:
        typedef detail::basic_nodes_collector<NodeT> base_class;

        bool VisitStmt(clang::Stmt* stmt)
        {
            NodeT* node = llvm::dyn_cast<NodeT>(stmt);
            if (node != nullptr)
            {
                this->nodes_.push_back(node);
            }
            return true;
        }
    };

    /// \brief Specialization for \c clang::Type nodes.
    template <typename NodeT>
    class nodes_collector<
        NodeT,
        typename llvm::enable_if<llvm::is_base_of<clang::Type, NodeT> >::
            type> : public detail::basic_nodes_collector<NodeT>,
                    public clang::RecursiveASTVisitor<nodes_collector<NodeT, typename llvm::enable_if<llvm::is_base_of<clang::Type, NodeT> >::type> >
    {
    public:
        typedef detail::basic_nodes_collector<NodeT> base_class;

        bool VisitType(clang::Type* type)
        {
            NodeT* node = llvm::dyn_cast<NodeT>(type);
            if (node != nullptr)
            {
                this->nodes_.push_back(node);
            }
            return true;
        }
    };

    /// \brief For-range loop support for \c begin.
    template <typename NodeT, typename DistinctizerT>
    auto begin(const nodes_collector<NodeT, DistinctizerT>& collector) -> decltype(collector.nodes_begin())
    {
        return collector.nodes_begin();
    }

    /// \brief For-range loop support for \c end.
    template <typename NodeT, typename DistinctizerT>
    auto end(const nodes_collector<NodeT, DistinctizerT>& collector) -> decltype(collector.nodes_end())
    {
        return collector.nodes_end();
    }

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_clang_utils.inl)

#endif // guard

/// \file bobopt_control_flow_search.hpp File contains definition of class
/// responsible for search and collecting values in particular part of code on
/// "must visit" paths and base policies definitions.

#ifndef BOBOPT_CLANG_CONTROL_FLOW_SEARCH_HPP_GUARD
#define BOBOPT_CLANG_CONTROL_FLOW_SEARCH_HPP_GUARD

#include <bobopt_config.hpp>
#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_language.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <iterator>
#include <map>
#include <type_traits>
#include <vector>

namespace bobopt
{

    // Configuration.
    //==========================================================================

    namespace detail
    {

        /// \brief Configuration group for control flow search algorithm.
        static config_group config("control_flow_search");
        /// \brief Configuration variable for loop body traversal.
        static config_variable<bool> config_loop_body(config, "search_loop_body", true);

    } // detail

    // heap_policy definition.
    //==========================================================================

    /// \brief Policy that creates Derived class instance from prototype
    /// using prototype design pattern.
    ///
    /// Derived class needs to implement prototype() member function that
    /// creates another instance of class that is NOT copy of object itself
    /// but has all values needed for code search.
    ///
    /// \tparam Derived Curiously recurring template pattern (CRTP).
    template <typename Derived>
    class heap_policy
    {
    public:
        // typedefs:
        typedef heap_policy<Derived> policy_type;
        typedef Derived derived_type;

        /// \brief Proxy returned by \c create_instance() member function.
        class proxy
        {
        public:
            proxy();

            derived_type& get();
            const derived_type& get() const;

            bool valid() const;

        private:
            friend heap_policy;
            derived_type* instance_;
        };

        typedef proxy instance_type;

        /// \brief Abstract member function for creation of another instance of prototype.
        virtual derived_type* prototype() const = 0;

        // instance creation/destruction:
        instance_type create_instance() const;
        void destroy_instance(instance_type& instance) const;

    protected:
        // protection:
        ~heap_policy();
    };

    // heap_policy implementation.
    //==========================================================================

    /// \brief Creates invalid object.
    template <typename Derived>
    BOBOPT_INLINE heap_policy<Derived>::proxy::proxy()
        : instance_(nullptr)
    {
    }

    /// \brief Access instance by non-const reference.
    template <typename Derived>
    BOBOPT_INLINE Derived& heap_policy<Derived>::proxy::get()
    {
        BOBOPT_ASSERT(valid());
        return *instance_;
    }

    /// \brief Access instance by const reference.
    template <typename Derived>
    BOBOPT_INLINE const Derived& heap_policy<Derived>::proxy::get() const
    {
        BOBOPT_ASSERT(valid());
        return *instance_;
    }

    /// \brief Indicates whether instance is valid and was created or not yet destroyed.
    template <typename Derived>
    BOBOPT_INLINE bool heap_policy<Derived>::proxy::valid() const
    {
        return (instance_ != nullptr);
    }

    /// \brief Don't delete through pointer to base.
    template <typename Derived>
    BOBOPT_INLINE heap_policy<Derived>::~heap_policy()
    {
    }

    /// \brief Create instance of derived object.
    template <typename Derived>
    BOBOPT_INLINE typename heap_policy<Derived>::instance_type heap_policy<Derived>::create_instance() const
    {
        instance_type instance;
        instance.instance_ = prototype();
        BOBOPT_ASSERT(instance.valid());
        return instance;
    }

    /// \brief Destroy instance of derived object.
    template <typename Derived>
    BOBOPT_INLINE void heap_policy<Derived>::destroy_instance(instance_type& instance) const
    {
        delete instance.instance_;
        instance.instance_ = nullptr;
    }

    // value_policy definition.
    //==========================================================================

    /// \brief Class that creates instance of Derived class using CRTP
    /// pattern.
    ///
    /// Instance of class object is not allocated on heap. It is copied to
    /// stack. Policy should be used for derived classes where it's easier
    /// to copy their instances and memory allocation can be unacceptable
    /// overhead.
    ///
    /// \note Derived class has to implement prototype() member function
    /// that return object of Derived class by value.
    ///
    /// \tparam Derived Curiously recurring template pattern (CRTP).
    template <typename Derived>
    class value_policy
    {
    public:
        // typedefs:
        typedef value_policy<Derived> policy_type;
        typedef Derived derived_type;

        /// \brief Proxy returned by create_instance() member function.
        class proxy
        {
        public:
            proxy();

            derived_type& get();
            const derived_type& get() const;

            bool valid() const;

        private:
            friend value_policy;
            bool valid_;
            derived_type instance_;
        };

        typedef proxy instance_type;

        // instance creation/destruction:
        instance_type create_instance() const;
        void destroy_instance(instance_type& instance) const;

    protected:
        // protection:
        ~value_policy();

    private:
        // access derived:
        derived_type& get_derived();
        const derived_type& get_derived() const;
    };

    // value_policy implementation.
    //==========================================================================

    /// \brief Creates invalid object.
    template <typename Derived>
    BOBOPT_INLINE value_policy<Derived>::proxy::proxy()
        : valid_(false)
        , instance_()
    {
    }

    /// \brief Access instance by non-const reference.
    template <typename Derived>
    BOBOPT_INLINE Derived& value_policy<Derived>::proxy::get()
    {
        BOBOPT_ASSERT(valid_);
        return instance_;
    }

    /// \brief Access instance by const reference.
    template <typename Derived>
    BOBOPT_INLINE const Derived& value_policy<Derived>::proxy::get() const
    {
        BOBOPT_ASSERT(valid_);
        return instance_;
    }

    /// \brief Indicates whether instance is valid and was created or not yet destroyed.
    template <typename Derived>
    BOBOPT_INLINE bool value_policy<Derived>::proxy::valid() const
    {
        return valid_;
    }

    /// \brief Don't delete through pointer to base.
    template <typename Derived>
    BOBOPT_INLINE value_policy<Derived>::~value_policy()
    {
    }

    /// \brief Create instance of derived object.
    template <typename Derived>
    BOBOPT_INLINE typename value_policy<Derived>::instance_type value_policy<Derived>::create_instance() const
    {
        instance_type instance;
        instance.instance_ = get_derived().prototype();
        instance.valid_ = true;
        return instance;
    }

    /// \brief Destroy instance of derived object.
    template <typename Derived>
    BOBOPT_INLINE void value_policy<Derived>::destroy_instance(instance_type& instance) const
    {
        instance.valid_ = false;
    }

    /// \brief Access object as object of Derived class by non-const reference.
    template <typename Derived>
    BOBOPT_INLINE Derived& value_policy<Derived>::get_derived()
    {
        return *static_cast<derived_type*>(this);
    }

    /// \brief Access object as object of Derived class by const reference.
    template <typename Derived>
    BOBOPT_INLINE const Derived& value_policy<Derived>::get_derived() const
    {
        return *static_cast<const derived_type*>(this);
    }

    // scoped_prototype definition.
    //==========================================================================

    /// \brief Programmer/exception protection for prototype instances using policies.
    ///
    /// Paired functions \c create_instance() and \c destroy_instance() indicates
    /// that there are place in code when instance doesn't have owner that will
    /// be able to release it in case of exception or programming mistake, and can
    /// cause leaks.
    ///
    /// Usage:
    /// \code
    /// class example : public heap_policy<example> {
    /// ...
    /// void function()
    /// {
    ///     scoped_prototype<example> instance(*this);
    ///     ...
    ///     // will be destroyed at the end of scope.
    ///     // or in case of exception.
    /// }
    /// ...
    /// };
    /// \endcode
    ///
    /// \tparam Derived Curiously recurring template pattern (CRTP).
    template <typename Derived>
    class scoped_prototype
    {
    public:
        // typedefs:
        typedef Derived derived_type;
        typedef typename Derived::instance_type instance_type;

        // create/destroy.
        explicit scoped_prototype(derived_type& derived, bool create_valid = true);
        ~scoped_prototype();

        // proxy:
        derived_type& get();
        const derived_type& get() const;
        bool valid() const;

        // manipulate:
        void create();
        void destroy();

        // raw:
        const instance_type& raw() const;

    private:
        // protection:
        scoped_prototype();
        BOBOPT_NONCOPYMOVABLE(scoped_prototype);

        // data members:
        derived_type& derived_;
        instance_type instance_;
    };

    // scoped_prototype implementation.
    //==========================================================================

    /// \brief Can be constructed only on valid \c Derived object.
    ///
    /// \param derived Reference to derived object.
    /// \param create_valid Whether instance should be created together with object construction.
    template <typename Derived>
    scoped_prototype<Derived>::scoped_prototype(derived_type& derived, bool create_valid)
        : derived_(derived)
        , instance_()
    {
        if (create_valid)
        {
            create();
        }
    }

    /// \brief Destructor always destroys instance even when it is invalid.
    template <typename Derived>
    scoped_prototype<Derived>::~scoped_prototype()
    {
        destroy();
    }

    /// \brief \b Proxy for \c instance_type. Access to derived object by non-const reference.
    template <typename Derived>
    BOBOPT_INLINE Derived& scoped_prototype<Derived>::get()
    {
        return instance_.get();
    }

    /// \brief \b Proxy for \c instance_type. Access to derived object by const reference.
    template <typename Derived>
    BOBOPT_INLINE const Derived& scoped_prototype<Derived>::get() const
    {
        return instance_.get();
    }

    /// \brief \b Proxy for \c instance_type. Indicates whether instance is valid.
    template <typename Derived>
    BOBOPT_INLINE bool scoped_prototype<Derived>::valid() const
    {
        return instance_.valid();
    }

    /// \brief Create new prototyped instance.
    ///
    /// Precondition: Current instance must be invalid. It can either be created as invalid
    ///               destroyed in lifetime using \c destroy() member function.
    template <typename Derived>
    BOBOPT_INLINE void scoped_prototype<Derived>::create()
    {
        BOBOPT_ASSERT(!valid());
        instance_ = derived_.create_instance();
    }

    /// \brief Destroy instance. No preconditions.
    template <typename Derived>
    BOBOPT_INLINE void scoped_prototype<Derived>::destroy()
    {
        derived_.destroy_instance(instance_);
    }

    /// \brief Raw access to \c instance_type.
    template <typename Derived>
    BOBOPT_INLINE const typename scoped_prototype<Derived>::instance_type& scoped_prototype<Derived>::raw() const
    {
        return instance_;
    }

    // control_flow_search.
    //==========================================================================

    /// \brief Class used to search values in code.
    ///
    /// Class handles tree traversal and holding of values. Therefore there
    /// are constraint for value type that it can be put in standard vector
    /// container. It also needs to compare values so it can make set union,
    /// intersection and unique.
    ///
    /// \tparam Derived Derived Curiously recurring template pattern (CRTP).
    /// \tparam Value Type of value that is searched in code.
    /// \tparam PrototypePolicy Tree traversal needs to create new objects
    ///         of derived class. Policy defines how are these new objects
    ///         created. Default they're created and passed by value.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy = value_policy>
    class control_flow_search : public clang::RecursiveASTVisitor<Derived>, public PrototypePolicy<Derived>
    {
    public:
        // typedefs:
        typedef control_flow_search<Derived, Value, PrototypePolicy> base_type;
        typedef Derived derived_type;
        typedef Value value_type;
        typedef PrototypePolicy<Derived> prototype_type;
        typedef typename PrototypePolicy<Derived>::instance_type instance_type;
        typedef std::vector<clang::ast_type_traits::DynTypedNode> locations_type;
        typedef std::vector<Value> values_type;

        // access derived:
        derived_type& get_derived();
        const derived_type& get_derived() const;

        // access results:
        values_type get_values() const;
        bool has_value(const value_type& val) const;
        locations_type get_locations(const value_type& val) const;
        std::pair<unsigned, unsigned> get_min_max(const value_type& val) const;

        // traversal:
        bool TraverseIfStmt(clang::IfStmt* if_stmt);
        bool VisitIfStmt(clang::IfStmt* if_stmt);

        bool TraverseForStmt(clang::ForStmt* for_stmt);
        bool VisitForStmt(clang::ForStmt* for_stmt);

        bool TraverseWhileStmt(clang::WhileStmt* while_stmt);
        bool VisitWhileStmt(clang::WhileStmt* while_stmt);

        bool TraverseSwitchStmt(clang::SwitchStmt* switch_stmt);
        bool VisitSwitchStmt(clang::SwitchStmt* switch_stmt);

        bool TraverseCXXTryStmt(clang::CXXTryStmt* try_stmt);
        bool VisitCXXTryStmt(clang::CXXTryStmt* try_stmt);

        bool TraverseBinLAnd(clang::BinaryOperator* binary_operator);
        bool TraverseBinLOr(clang::BinaryOperator* binary_operator);

        bool VisitBreakStmt(clang::BreakStmt* break_stmt);
        bool VisitContinueStmt(clang::ContinueStmt* continue_stmt);
        bool VisitReturnStmt(clang::ReturnStmt* return_stmt);

    protected:
        // create/destroy:
        explicit control_flow_search(clang::ASTContext* context = nullptr);
        ~control_flow_search();

        // value management:
        void insert_value(const value_type& val);
        void insert_value_location(const value_type& val, clang::ast_type_traits::DynTypedNode location);
        void remove_value(const value_type& val);

        clang::ASTContext* context_;

    private:
        // helpers:

        /// \brief Flags for handling traversal situations.
        enum cff_flags
        {
            cff_break = 1 << 0,
            cff_continue = 1 << 1,
            cff_return = 1 << 2
        };

        // typedefs:
        struct value_info
        {
            locations_type locations;
            unsigned min;
            unsigned max;
        };

        typedef std::map<Value, value_info> container_type;

        // traversal helpers:
        bool traverse_for_body(clang::ForStmt* for_stmt) const;
        bool traverse_while_body(clang::WhileStmt* while_stmt) const;
        bool should_continue() const;

        // value helpers:
        const container_type& get_container() const;
        void append_visitor(const container_type& values);

        static container_type make_intersection(const container_type& lhs, const container_type& rhs);
        static container_type make_union(const container_type& lhs, const container_type& rhs);

        container_type values_map_;
        int flags_;
    };

    // control_flow_search implementation.
    //==========================================================================

    /// \brief Access object as non-const reference to Derived.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE Derived& control_flow_search<Derived, Value, PrototypePolicy>::get_derived()
    {
        return *static_cast<Derived*>(this);
    }

    /// \brief Access object as const reference to Derived.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE const Derived& control_flow_search<Derived, Value, PrototypePolicy>::get_derived() const
    {
        return *static_cast<const Derived*>(this);
    }

    /// \brief Access values collected by search.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::values_type
    control_flow_search<Derived, Value, PrototypePolicy>::get_values() const
    {
        values_type result;
        result.reserve(values_map_.size());

        for (const auto& it : values_map_)
        {
            result.push_back(it.first);
        }

        return result;
    }

    /// \brief Ask whether value was found in search.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::has_value(const value_type& val) const
    {
        return (values_map_.find(val) != std::end(values_map_));
    }

    /// \brief Access locations for chosen value.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::locations_type
    control_flow_search<Derived, Value, PrototypePolicy>::get_locations(const value_type& val) const
    {
        auto found = values_map_.find(val);
        BOBOPT_ASSERT(found != std::end(values_map_));
        return found->second.locations;
    }

    /// \brief Get minimum and maximum of value occurances on paths.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE std::pair<unsigned, unsigned> control_flow_search<Derived, Value, PrototypePolicy>::get_min_max(const value_type& val) const
    {
        auto found = values_map_.find(val);
        BOBOPT_ASSERT(found != std::end(values_map_));
        return std::make_pair(found->second.min, found->second.max);
    }

    /// \brief Recursive traversal of if statement will be handled by VisitIfStmt member function.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseIfStmt(clang::IfStmt* if_stmt)
    {
        return this->WalkUpFromIfStmt(if_stmt) && should_continue();
    }

    /// \brief Function will handle recursive traversal and value evaluation of if statement.
    ///
    /// Condition of if statement will be always passed so values are added automatically.
    /// One new prototype is created for then branch, one for else branch. They're both
    /// run on related parts of AST. In the final, function will create instersection
    /// of found values.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitIfStmt(clang::IfStmt* if_stmt)
    {
        // Condition is always in control flow path.
        clang::Expr* cond_expr = if_stmt->getCond();
        BOBOPT_ASSERT(cond_expr != nullptr);

        scoped_prototype<control_flow_search> cond_visitor(*this);
        cond_visitor.get().TraverseStmt(cond_expr);
        flags_ |= cond_visitor.get().flags_;

        // Then branch.
        scoped_prototype<control_flow_search> then_visitor(*this, false);
        clang::Stmt* then_stmt = if_stmt->getThen();
        if (then_stmt != nullptr)
        {
            then_visitor.create();
            BOBOPT_ASSERT(then_visitor.valid());
            then_visitor.get().TraverseStmt(then_stmt);
            flags_ |= then_visitor.get().flags_;
        }

        // Else branch.
        scoped_prototype<control_flow_search> else_visitor(*this, false);
        clang::Stmt* else_stmt = if_stmt->getElse();
        if (else_stmt != nullptr)
        {
            else_visitor.create();
            BOBOPT_ASSERT(else_visitor.valid());
            else_visitor.get().TraverseStmt(else_stmt);
            flags_ |= else_visitor.get().flags_;
        }

        // Store results of recursive traversal.
        container_type values = cond_visitor.get().get_container();
        if (then_visitor.valid())
        {
            container_type then_values = then_visitor.get().get_container();
            if (else_visitor.valid())
            {
                container_type else_values = else_visitor.get().get_container();
                container_type both_values = make_intersection(then_values, else_values);
                values = make_union(values, both_values);
            }
            else
            {
                values = make_union(values, then_values);
            }
        }
        else
        {
            if (else_visitor.valid())
            {
                container_type else_values = else_visitor.get().get_container();
                values = make_union(values, else_values);
            }
        }

        append_visitor(values);
        return true;
    }

    /// \brief Recursive traversal of for statement will be handled by VisitForStmt member function.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseForStmt(clang::ForStmt* for_stmt)
    {
        bool result = this->WalkUpFromForStmt(for_stmt);
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Function will handle recursive traversal and value evaluation of for statement.
    ///
    /// There are only two parts of for statement that will always be evaluated at least
    /// once. It's initial statement and condition expression. Values found in both parts
    /// are added to set of values found.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitForStmt(clang::ForStmt* for_stmt)
    {
        // Init statement.
        clang::Stmt* init_stmt = for_stmt->getInit();
        scoped_prototype<control_flow_search> init_visitor(*this, false);
        if (init_stmt != nullptr)
        {
            init_visitor.create();
            BOBOPT_ASSERT(init_visitor.valid());
            init_visitor.get().TraverseStmt(init_stmt);
            flags_ |= init_visitor.get().flags_;
        }

        // Condition expression.
        clang::Expr* cond_expr = for_stmt->getCond();
        scoped_prototype<control_flow_search> cond_visitor(*this, false);
        if (cond_expr != nullptr)
        {
            cond_visitor.create();
            BOBOPT_ASSERT(cond_visitor.valid());
            cond_visitor.get().TraverseStmt(cond_expr);
            flags_ |= cond_visitor.get().flags_;
        }

        // Deeper analysis.
        scoped_prototype<control_flow_search> incr_visitor(*this, false);
        scoped_prototype<control_flow_search> body_visitor(*this, false);
        if (traverse_for_body(for_stmt))
        {
            clang::Stmt* body_stmt = for_stmt->getBody();
            if (body_stmt != nullptr)
            {
                body_visitor.create();
                BOBOPT_ASSERT(body_visitor.valid());
                body_visitor.get().TraverseStmt(body_stmt);
                flags_ |= body_visitor.get().flags_;
            }

            clang::Expr* incr_expr = for_stmt->getInc();
            if (incr_expr != nullptr)
            {
                incr_visitor.create();
                BOBOPT_ASSERT(incr_visitor.valid());
                incr_visitor.get().TraverseStmt(incr_expr);
                flags_ |= incr_visitor.get().flags_;
            }
        }

        // Collect values.
        if (init_visitor.valid())
        {
            append_visitor(init_visitor.get().get_container());
        }

        if (cond_visitor.valid())
        {
            append_visitor(cond_visitor.get().get_container());
        }

        if (incr_visitor.valid())
        {
            append_visitor(incr_visitor.get().get_container());
        }

        if (body_visitor.valid())
        {
            append_visitor(body_visitor.get().get_container());
        }

        return true;
    }

    /// \brief Recursive traversal of while statement will be handled by VisitWhileStmt member function.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseWhileStmt(clang::WhileStmt* while_stmt)
    {
        bool result = this->WalkUpFromWhileStmt(while_stmt);
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Function will handle recursive traversal and value evaluation of while statement.
    ///
    /// Only condition is always evaluated. Its values are added to result set of values.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitWhileStmt(clang::WhileStmt* while_stmt)
    {
        // Condition expression.
        clang::Expr* cond_expr = while_stmt->getCond();
        scoped_prototype<control_flow_search> cond_visitor(*this, false);
        if (cond_expr != nullptr)
        {
            cond_visitor.create();
            BOBOPT_ASSERT(cond_visitor.valid());
            cond_visitor.get().TraverseStmt(cond_expr);
            flags_ |= cond_visitor.get().flags_;
        }

        // Deeper analysis.
        scoped_prototype<control_flow_search> body_visitor(*this, false);
        if (traverse_while_body(while_stmt))
        {
            clang::Stmt* body_stmt = while_stmt->getBody();
            if (body_stmt != nullptr)
            {
                body_visitor.create();
                BOBOPT_ASSERT(body_visitor.valid());
                body_visitor.get().TraverseStmt(body_stmt);
                flags_ |= body_visitor.get().flags_;
            }
        }

        // Collect values.
        if (cond_visitor.valid())
        {
            append_visitor(cond_visitor.get().get_container());
        }

        if (body_visitor.valid())
        {
            append_visitor(body_visitor.get().get_container());
        }

        return true;
    }

    /// \brief Recursive traversal of switch statement will be handled by VisitSwitchStmt member function.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseSwitchStmt(clang::SwitchStmt* switch_stmt)
    {
        bool result = this->WalkUpFromSwitchStmt(switch_stmt);
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Function will handle recursive traversal and value evaluation of switch statement.
    ///
    /// Only condition is always evaluated. Its values are added to result set of values.
    /// Case statements are way more complex to handle, but basic principle is the same
    /// as for if statement.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitSwitchStmt(clang::SwitchStmt* switch_stmt)
    {
        clang::Expr* cond_expr = switch_stmt->getCond();
        BOBOPT_ASSERT(cond_expr != nullptr);

        scoped_prototype<control_flow_search> cond_visitor(*this);
        BOBOPT_ASSERT(cond_visitor.valid());
        cond_visitor.get().TraverseStmt(cond_expr);
        flags_ |= cond_visitor.get().flags_;

        if (cond_visitor.valid())
        {
            append_visitor(cond_visitor.get().get_container());
        }

        return true;
    }

    /// \brief Recursive traversal of try statement will be handled by VisitCXXTryStmt member function.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseCXXTryStmt(clang::CXXTryStmt* try_stmt)
    {
        bool result = this->WalkUpFromCXXTryStmt(try_stmt);
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Function will handle recursive traversal and value evaluation of try statement.
    ///
    /// Try block is the only evaluated. Catch statements should not contain any values
    /// client is looking for.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitCXXTryStmt(clang::CXXTryStmt* try_stmt)
    {
        clang::Stmt* try_block = try_stmt->getTryBlock();
        if (try_block != nullptr)
        {
            scoped_prototype<control_flow_search> block_visitor(*this);
            BOBOPT_ASSERT(block_visitor.valid());
            block_visitor.get().TraverseStmt(try_block);
            flags_ |= block_visitor.get().flags_;

            if (block_visitor.valid())
            {
                append_visitor(block_visitor.get().get_container());
            }
        }

        return true;
    }

    /// \brief Recursive traversal of logical AND will forward job only to LHS.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseBinLAnd(clang::BinaryOperator* binary_operator)
    {
        bool result = this->WalkUpFromBinaryOperator(binary_operator);
        result &= this->TraverseStmt(binary_operator->getLHS());
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Recursive traversal of logical OR will forward job only to LHS.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseBinLOr(clang::BinaryOperator* binary_operator)
    {
        bool result = this->WalkUpFromBinaryOperator(binary_operator);
        result &= this->TraverseStmt(binary_operator->getLHS());
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Handles \c break; statement by finishing traversal and setting flag.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitBreakStmt(clang::BreakStmt*)
    {
        flags_ |= cff_break;
        return false;
    }

    /// \brief Handles \c continue; statement by finishing traversal and setting flag.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitContinueStmt(clang::ContinueStmt*)
    {
        flags_ |= cff_continue;
        return false;
    }

    /// \brief Handles \c return; statement by finishing traversal and setting flag.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    bool control_flow_search<Derived, Value, PrototypePolicy>::VisitReturnStmt(clang::ReturnStmt*)
    {
        flags_ |= cff_return;
        return false;
    }

    /// \brief Don't create instances of base.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE control_flow_search<Derived, Value, PrototypePolicy>::control_flow_search(clang::ASTContext* context)
        : context_(context)
        , values_map_()
        , flags_(0)
    {
    }

    /// \brief Don't delete through pointer to base.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE control_flow_search<Derived, Value, PrototypePolicy>::~control_flow_search()
    {
    }

    /// \brief Way for derived class to insert value into container.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::insert_value(const value_type& val)
    {
        if (values_map_.find(val) == std::end(values_map_))
        {
            value_info info;
            info.min = 1;
            info.max = 1;
            values_map_.insert(std::make_pair(val, std::move(info)));
        }
    }

    /// \brief Way for derived class to insert value together with location where it was found into container.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::insert_value_location(const value_type& val,
                                                                                                   clang::ast_type_traits::DynTypedNode location)
    {
        auto found = values_map_.find(val);
        if (found == std::end(values_map_))
        {
            value_info info;
            info.locations.emplace_back(std::move(location));
            info.min = 1;
            info.max = 1;
            values_map_.insert(std::make_pair(val, std::move(info)));
        }
        else
        {
            found->second.locations.push_back(location);
            ++found->second.min;
            ++found->second.max;
        }
    }

    /// \brief Way for derived class to remove value from container.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::remove_value(const value_type& val)
    {
        values_map_.erase(val);
    }

    /// \brief Function that evaluates whether for statement body will be executed at least once.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::traverse_for_body(clang::ForStmt* for_stmt) const
    {
        clang::DeclStmt* decl_stmt = llvm::dyn_cast_or_null<clang::DeclStmt>(for_stmt->getInit());
        if ((decl_stmt == nullptr) || !decl_stmt->isSingleDecl())
        {
            return false;
        }

        clang::VarDecl* var_decl = llvm::dyn_cast<clang::VarDecl>(decl_stmt->getSingleDecl());
        if ((var_decl == nullptr) || !var_decl->checkInitIsICE())
        {
            return false;
        }

        clang::Expr* cond_expr = for_stmt->getCond();
        if (cond_expr == nullptr)
        {
            return false;
        }

        const bool is_constexpr = var_decl->isConstexpr();
        var_decl->setConstexpr(true);
        bool eval = detail::config_loop_body.get();
        BOBOPT_ASSERT(context_ != nullptr);
        if (!cond_expr->EvaluateAsBooleanCondition(eval, *context_))
        {
            var_decl->setConstexpr(is_constexpr);
            return false;
        }

        var_decl->setConstexpr(is_constexpr);
        return eval;
    }

    /// \brief Function used for deeper static analysis. It tries to evaluates if expression
    /// will be at least once true and body will be executed.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::traverse_while_body(clang::WhileStmt* while_stmt) const
    {
        BOBOPT_UNUSED_EXPRESSION(while_stmt);
        return detail::config_loop_body.get();
    }

    /// \brief Function indicates whether traversal should continue based on flags.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::should_continue() const
    {
        bool result = true;
        result &= ((flags_ & cff_break) == 0);
        result &= ((flags_ & cff_continue) == 0);
        result &= ((flags_ & cff_return) == 0);
        return result;
    }

    /// \brief Get access to visitor storage.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE const typename control_flow_search<Derived, Value, PrototypePolicy>::container_type&
    control_flow_search<Derived, Value, PrototypePolicy>::get_container() const
    {
        return values_map_;
    }

    /// \brief Append values from container to this visitor instance and copy their locations from visitor instance.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::append_visitor(const container_type& values)
    {
        for (const auto& val : values)
        {
            auto result = values_map_.insert(std::make_pair(val.first, value_info()));
            if (result.second)
            {
                result.first->second.locations = val.second.locations;
                result.first->second.min = val.second.min;
                result.first->second.max = val.second.max;
            }
            else
            {
                append(result.first->second.locations, val.second.locations);
                result.first->second.min += val.second.min;
                result.first->second.max += val.second.max;
            }
        }
    }

    /// \brief Create container with intersection of values from two containers.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::container_type
    control_flow_search<Derived, Value, PrototypePolicy>::make_intersection(const container_type& lhs, const container_type& rhs)
    {
        container_type result;

        auto lhs_it = lhs.begin();
        auto rhs_it = rhs.begin();

        while ((lhs_it != std::end(lhs)) && (rhs_it != std::end(rhs)))
        {
            if (lhs_it->first < rhs_it->first)
            {
                ++lhs_it;
                continue;
            }

            if (rhs_it->first < lhs_it->first)
            {
                ++rhs_it;
                continue;
            }

            value_info info;
            append(info.locations, lhs_it->second.locations);
            append(info.locations, rhs_it->second.locations);
            info.min = std::min(lhs_it->second.min, rhs_it->second.min);
            info.max = std::max(lhs_it->second.max, rhs_it->second.max);
            result.insert(std::make_pair(lhs_it->first, std::move(info)));

            ++lhs_it;
            ++rhs_it;
        }

        return result;
    }

    /// \brief Create container as union of values from two containers.
    template <typename Derived, typename Value, template <typename> class PrototypePolicy>
    BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::container_type
    control_flow_search<Derived, Value, PrototypePolicy>::make_union(const container_type& lhs, const container_type& rhs)
    {
        container_type result;

        auto lhs_it = lhs.begin();
        auto rhs_it = rhs.begin();

        for (; (lhs_it != std::end(lhs)) && (rhs_it != std::end(rhs)); ++lhs_it, ++rhs_it)
        {
            if (lhs_it->first < rhs_it->first)
            {
                result.insert(std::make_pair(lhs_it->first, lhs_it->second));
                continue;
            }

            if (rhs_it->first < lhs_it->first)
            {
                result.insert(std::make_pair(rhs_it->first, rhs_it->second));
                continue;
            }

            value_info info;
            append(info.locations, lhs_it->second.locations);
            append(info.locations, rhs_it->second.locations);
            info.min = lhs_it->second.min + rhs_it->second.min;
            info.max = lhs_it->second.max + rhs_it->second.max;
            result.insert(std::make_pair(lhs_it->first, std::move(info)));
        }

        for (; lhs_it != std::end(lhs); ++lhs_it)
        {
            result.insert(std::make_pair(lhs_it->first, lhs_it->second));
        }

        for (; rhs_it != std::end(rhs); ++rhs_it)
        {
            result.insert(std::make_pair(rhs_it->first, rhs_it->second));
        }

        return result;
    }

} // namespace

#endif // guard

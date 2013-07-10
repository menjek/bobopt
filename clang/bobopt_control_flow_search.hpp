/// \file bobopt_control_flow_search.hpp File contains definition of class responsible for search and collecting
/// values in particular part of code on "must visit" paths and base policies definitions.

#ifndef BOBOPT_CLANG_CONTROL_FLOW_SEARCH_HPP_GUARD
#define BOBOPT_CLANG_CONTROL_FLOW_SEARCH_HPP_GUARD

#include <bobopt_debug.hpp>
#include <bobopt_language.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <iterator>
#include <map>
#include <type_traits>
#include <vector>

namespace bobopt {

	// clone_policy definition.
	//==============================================================================

	/// \brief Policy that creates Derived class instance from prototype
	/// using prototype design pattern.
	///
	/// Derived class needs to implement prototype() member function that
	/// creates another instance of class that is NOT copy of object itself
	/// but has all values needed for code search.
	///
	/// \tparam Derived Curiously recurring template pattern (CRTP).
	template<typename Derived>
	class heap_policy
	{
	public:
		// typedefs:
		typedef heap_policy<Derived> policy_type;
		typedef Derived derived_type;

		/// \brief Proxy returned by created_instance() member function.
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

	protected:
		~heap_policy();

		// instance creation/destruction:
		instance_type create_instance() const;
		void destroy_instance(instance_type& instance) const;
	};

	// heap_policy implementation.
	//==============================================================================

	/// \brief Creates invalid object.
	template<typename Derived>
	BOBOPT_INLINE heap_policy<Derived>::proxy::proxy()
		: instance_(nullptr)
	{}
	
	/// \brief Access instance by non-const reference.
	template<typename Derived>
	BOBOPT_INLINE Derived& heap_policy<Derived>::proxy::get()
	{
		BOBOPT_ASSERT(valid());
		return *instance_;
	}

	/// \brief Access instance by const reference.
	template<typename Derived>
	BOBOPT_INLINE const Derived& heap_policy<Derived>::proxy::get() const
	{
		BOBOPT_ASSERT(valid());
		return *instance_;
	}

	/// \brief Indicates whether instance is valid and was created or not yet destroyed.
	template<typename Derived>
	BOBOPT_INLINE bool heap_policy<Derived>::proxy::valid() const
	{
		return (instance_ != nullptr);
	}

	/// \brief Don't delete through pointer to base.
	template<typename Derived>
	BOBOPT_INLINE heap_policy<Derived>::~heap_policy()
	{}

	/// \brief Create instance of derived object.
	template<typename Derived>
	BOBOPT_INLINE typename heap_policy<Derived>::instance_type heap_policy<Derived>::create_instance() const
	{
		instance_type instance;
		instance.instance_ = prototype();
		BOBOPT_ASSERT(instance.valid());
		return instance;
	}

	/// \brief Destroy instance of derived object.
	template<typename Derived>
	BOBOPT_INLINE void heap_policy<Derived>::destroy_instance(instance_type& instance) const
	{
		delete instance.instance_;
		instance.instance_ = nullptr;
	}

	// value_policy definition.
	//==============================================================================

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
	template<typename Derived>
	class value_policy
	{
	public:
		// typedefs:
		typedef value_policy<Derived> policy_type;
		typedef Derived derived_type;

		/// \brief Proxy returned by created_instance() member function.
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

	protected:
		~value_policy();
		
		// instance creation/destruction:
		instance_type create_instance() const;
		void destroy_instance(instance_type& instance) const;

	private:
		// access derived:
		derived_type& get_derived();
		const derived_type& get_derived() const;
	};


	// value_policy implementation.
	//==============================================================================

	/// \brief Creates invalid object.
	template<typename Derived>
	BOBOPT_INLINE value_policy<Derived>::proxy::proxy()
		: valid_(false)
		, instance_()
	{}

	/// \brief Access instance by non-const reference.
	template<typename Derived>
	BOBOPT_INLINE Derived& value_policy<Derived>::proxy::get()
	{
		BOBOPT_ASSERT(valid_);
		return instance_;
	}

	/// \brief Access instance by const reference.
	template<typename Derived>
	BOBOPT_INLINE const Derived& value_policy<Derived>::proxy::get() const
	{
		BOBOPT_ASSERT(valid_);
		return instance_;
	}

	/// \brief Indicates whether instance is valid and was created or not yet destroyed.
	template<typename Derived>
	BOBOPT_INLINE bool value_policy<Derived>::proxy::valid() const
	{
		return valid_;
	}

	/// \brief Don't delete through pointer to base.
	template<typename Derived>
	BOBOPT_INLINE value_policy<Derived>::~value_policy()
	{}

	/// \brief Create instance of derived object.
	template<typename Derived>
	BOBOPT_INLINE typename value_policy<Derived>::instance_type value_policy<Derived>::create_instance() const
	{
		instance_type instance;
		instance.instance_ = get_derived().prototype();
		instance.valid_ = true;
		return instance;
	}

	/// \brief Destroy instance of derived object.
	template<typename Derived>
	BOBOPT_INLINE void value_policy<Derived>::destroy_instance(instance_type& instance) const
	{
		instance.valid_ = false;
	}

	/// \brief Access object as object of Derived class by non-const reference.
	template<typename Derived>
	BOBOPT_INLINE Derived& value_policy<Derived>::get_derived()
	{
		return *static_cast<derived_type*>(this);
	}

	/// \brief Access object as object of Derived class by const reference.
	template<typename Derived>
	BOBOPT_INLINE const Derived& value_policy<Derived>::get_derived() const
	{
		return *static_cast<const derived_type*>(this);
	}

	// control_flow_search.
	//==============================================================================

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
	template<typename Derived, typename Value, template <typename> class PrototypePolicy = value_policy>
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

		bool TraverseBinaryOperator(clang::BinaryOperator* binary_operator);
		bool VisitBinaryOperator(clang::BinaryOperator* binary_operator);

	protected:

		// create/destroy:
		control_flow_search();
		~control_flow_search();
		
		// value management:
		void insert_value(const value_type& val);
		void insert_value_location(const value_type& val, clang::ast_type_traits::DynTypedNode location);
		void remove_value(const value_type& val);

	private:

		// typedefs:
		typedef std::map<Value, locations_type> container_type;

		// traversal helpers:
		bool should_traverse_body(clang::Expr* expr) const;

		// value helpers:
		void append_values(const instance_type& visitor);
		void append_values(const values_type& values);
		void append_values_locations(const values_type& values, const instance_type& visitor);

		static void make_unique(values_type& values);
		static values_type make_intersection(values_type lhs, values_type rhs);
		static values_type make_union(values_type lhs, const values_type& rhs);

		container_type values_map_;
	};

	// control_flow_search implementation.
	//==============================================================================

	/// \brief Access object as non-const reference to Derived.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE Derived& control_flow_search<Derived, Value, PrototypePolicy>::get_derived()
	{
		return *static_cast<Derived*>(this);
	}

	/// \brief Access object as const reference to Derived.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE const Derived& control_flow_search<Derived, Value, PrototypePolicy>::get_derived() const
	{
		return *static_cast<const Derived*>(this);
	}

	/// \brief Access values collected by search.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::values_type control_flow_search<Derived, Value, PrototypePolicy>::get_values() const
	{
		using namespace std;

		values_type result;
		result.reserve(values_map_.size());

		for (const auto& it : values_map_)
		{
			result.push_back(it.first);
		}

		return result;
	}

	/// \brief Ask whether value was found in search.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::has_value(const value_type& val) const
	{
		return (values_map_.find(val) != end(values_map_));
	}

	/// \brief Access locations for chosen value.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::locations_type control_flow_search<Derived, Value, PrototypePolicy>::get_locations(const value_type& val) const
	{
		auto found = values_map_.find(val);
		BOBOPT_ASSERT(found != end(values_map_));
		return found->second;
	}

	/// \brief Recursive traversal of if statement will be handled by VisitIfStmt member function.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseIfStmt(clang::IfStmt* if_stmt)
	{
		return VisitIfStmt(if_stmt);
	}

	/// \brief Function will handle recursive traversal and value evaluation of if statement.
	///
	/// Condition of if statement will be always passed so values are added automatically.
	/// One new prototype is created for then branch, one for else branch. They're both
	/// run on related parts of AST. In the final, function will create instersection
	/// of found values.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	bool control_flow_search<Derived, Value, PrototypePolicy>::VisitIfStmt(clang::IfStmt* if_stmt)
	{
		// Condition is always in control flow path.
		clang::Expr* cond_expr = if_stmt->getCond();
		BOBOPT_ASSERT(cond_expr != nullptr);

		instance_type cond_visitor = prototype_type::create_instance();
 		cond_visitor.get().TraverseStmt(cond_expr);

		// Then branch.
		instance_type then_visitor;
		clang::Stmt* then_stmt = if_stmt->getThen();
		if (then_stmt != nullptr)
		{
			then_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(then_visitor.valid());
			then_visitor.get().TraverseStmt(then_stmt);
		}

		// Else branch.
		instance_type else_visitor;
		clang::Stmt* else_stmt = if_stmt->getElse();
		if (else_stmt != nullptr)
		{
			else_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(else_visitor.valid());
			else_visitor.get().TraverseStmt(else_stmt);
		}

		// Store results of recursive traversal.
		values_type values = cond_visitor.get().get_values();

		using namespace std;
		if (then_visitor.valid())
		{
			values_type then_values = then_visitor.get().get_values();
			if (else_visitor.valid())
			{
				values_type else_values = else_visitor.get().get_values();
				values_type both_values = make_intersection(then_values, else_values);
				copy(begin(both_values), end(both_values), back_inserter(values));
			}
			else
			{
				copy(begin(then_values), end(then_values), back_inserter(values));
			}
		}
		else
		{
			if (else_visitor.valid())
			{
				values_type else_values = else_visitor.get().get_values();
				copy(begin(else_values), end(else_values), back_inserter(values));
			}
		}

		append_values_locations(values, cond_visitor);
		append_values_locations(values, then_visitor);
		append_values_locations(values, else_visitor);

		// Cleanup.
		prototype_type::destroy_instance(cond_visitor);
		prototype_type::destroy_instance(then_visitor);
		prototype_type::destroy_instance(else_visitor);

		return true;
	}

	/// \brief Recursive traversal of for statement will be handled by VisitForStmt member function.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseForStmt(clang::ForStmt* for_stmt)
	{
		return VisitForStmt(for_stmt);
	}

	/// \brief Function will handle recursive traversal and value evaluation of for statement.
	///
	/// There are only two parts of for statement that will always be evaluated at least
	/// once. It's initial statement and condition expression. Values found in both parts
	/// are added to set of values found.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	bool control_flow_search<Derived, Value, PrototypePolicy>::VisitForStmt(clang::ForStmt* for_stmt)
	{
		// Init statement.
		clang::Stmt* init_stmt = for_stmt->getInit();
		instance_type init_visitor;
		if (init_stmt != nullptr)
		{
			init_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(init_visitor.valid());
			init_visitor.get().TraverseStmt(init_stmt);
		}

		// Condition expression.
		clang::Expr* cond_expr = for_stmt->getCond();
		instance_type cond_visitor;
		if (cond_expr != nullptr)
		{
			cond_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(cond_visitor.valid());
			cond_visitor.get().TraverseStmt(cond_expr);
		}

		// Deeper analysis.
		instance_type incr_visitor;
		instance_type body_visitor;
		if (should_traverse_body(cond_expr))
		{
			clang::Stmt* body_stmt = for_stmt->getBody();
			if (body_stmt != nullptr)
			{
				body_visitor = prototype_type::create_instance();
				BOBOPT_ASSERT(body_visitor.valid());
				body_visitor.get().TraverseStmt(body_stmt);
			}

			clang::Expr* incr_expr = for_stmt->getInc();
			if (incr_expr != nullptr)
			{
				incr_visitor = prototype_type::create_instance();
				BOBOPT_ASSERT(incr_visitor.valid());
				incr_visitor.get().TraverseStmt(incr_expr);
			}
		}

		// Collect values.
		append_values(init_visitor);
		append_values(cond_visitor);
		append_values(incr_visitor);
		append_values(body_visitor);

		// Cleanup.
		prototype_type::destroy_instance(init_visitor);
		prototype_type::destroy_instance(cond_visitor);
		prototype_type::destroy_instance(incr_visitor);
		prototype_type::destroy_instance(body_visitor);

		return true;
	}

	/// \brief Recursive traversal of while statement will be handled by VisitWhileStmt member function.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseWhileStmt(clang::WhileStmt* while_stmt)
	{
		return VisitWhileStmt(while_stmt);
	}

	/// \brief Function will handle recursive traversal and value evaluation of while statement.
	///
	/// Only condition is always evaluated. Its values are added to result set of values.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	bool control_flow_search<Derived, Value, PrototypePolicy>::VisitWhileStmt(clang::WhileStmt* while_stmt)
	{
		// Condition expression.
		clang::Expr* cond_expr = while_stmt->getCond();
		instance_type cond_visitor;
		if (cond_expr != nullptr)
		{
			cond_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(cond_visitor.valid());
			cond_visitor.get().TraverseStmt(cond_expr);
		}

		// Deeper analysis.
		instance_type body_visitor;
		if (should_traverse_body(cond_expr))
		{
			clang::Stmt* body_stmt = while_stmt->getBody();
			if (body_stmt != nullptr)
			{
				body_visitor = prototype_type::create_instance();
				BOBOPT_ASSERT(body_visitor.valid());
				body_visitor.get().TraverseStmt(body_stmt);
			}
		}

		// Collect values.
		append_values(cond_visitor);
		append_values(body_visitor);

		// Cleanup.
		prototype_type::destroy_instance(cond_visitor);
		prototype_type::destroy_instance(body_visitor);

		return true;
	}

	/// \brief Recursive traversal of switch statement will be handled by VisitSwitchStmt member function.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseSwitchStmt(clang::SwitchStmt* switch_stmt)
	{
		return VisitSwitchStmt(switch_stmt);
	}

	/// \brief Function will handle recursive traversal and value evaluation of switch statement.
	///
	/// Only condition is always evaluated. Its values are added to result set of values.
	/// Case statements are way more complex to handle, but basic principle is the same
	/// as for if statement.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	bool control_flow_search<Derived, Value, PrototypePolicy>::VisitSwitchStmt(clang::SwitchStmt* switch_stmt)
	{
		clang::Expr* cond_expr = switch_stmt->getCond();
		BOBOPT_ASSERT(cond_expr != nullptr);

		instance_type cond_visitor = prototype_type::create_instance();
		BOBOPT_ASSERT(cond_visitor.valid());
		cond_visitor.get().TraverseStmt(cond_expr);

		append_values(cond_visitor);
			
		prototype_type::destroy_instance(cond_visitor);

		return true;
	}

	/// \brief Recursive traversal of try statement will be handled by VisitCXXTryStmt member function.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseCXXTryStmt(clang::CXXTryStmt* try_stmt)
	{
		return VisitCXXTryStmt(try_stmt);
	}

	/// \brief Function will handle recursive traversal and value evaluation of try statement.
	///
	/// Try block is the only evaluated. Catch statements should not contain any values
	/// client is looking for.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	bool control_flow_search<Derived, Value, PrototypePolicy>::VisitCXXTryStmt(clang::CXXTryStmt* try_stmt)
	{
		clang::Stmt* try_block = try_stmt->getTryBlock();
		if (try_block != nullptr)
		{
			instance_type block_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(block_visitor.valid());
			block_visitor.get().TraverseStmt(try_block);

			append_values(block_visitor);

			prototype_type::destroy_instance(block_visitor);
		}

		return true;
	}

	/// \brief Recursive traversal of binary operator expression will be handled by VisitBinaryOperator member function.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::TraverseBinaryOperator(clang::BinaryOperator* binary_operator)
	{
		if (!binary_operator->isLogicalOp())
		{
			return base_type::TraverseBinaryOperator(binary_operator);
		}

		return VisitBinaryOperator(binary_operator);
	}

	/// \brief Function will handle recursive traversal and value evaluation of for statement.
	///
	/// Since there's short-circuit evaluation of binary logical expression, only first expression
	/// is search for values.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	bool control_flow_search<Derived, Value, PrototypePolicy>::VisitBinaryOperator(clang::BinaryOperator* binary_operator)
	{
		if (binary_operator->isLogicalOp())
		{
			clang::Expr* lhs_expr = binary_operator->getLHS();
			BOBOPT_ASSERT(lhs_expr != nullptr);

			instance_type lhs_visitor = prototype_type::create_instance();
			BOBOPT_ASSERT(lhs_visitor.valid());
			lhs_visitor.get().TraverseStmt(lhs_expr);

			append_values(lhs_visitor);
		}

		return true;
	}

	/// \brief Don't create instances of base.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE control_flow_search<Derived, Value, PrototypePolicy>::control_flow_search()
		: values_map_()
	{}

	/// \brief Don't delete through pointer to base.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE control_flow_search<Derived, Value, PrototypePolicy>::~control_flow_search()
	{}

	/// \brief Way for derived class to insert value into container.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::insert_value(const value_type& val)
	{
		using namespace std;

		if (values_map_.find(val) == end(values_map_))
		{
			values_map_.insert(make_pair(val, locations_type()));
		}
	}

	/// \brief Way for derived class to insert value together with location where it was found into container.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::insert_value_location(const value_type& val, clang::ast_type_traits::DynTypedNode location)
	{
		using namespace std;

		auto found = values_map_.find(val);
		if (found == end(values_map_))
		{
			locations_type locations(1, location);
			values_map_.insert(make_pair(val, move(locations)));
		}
		else
		{
			found->second.push_back(location);
		}
	}

	/// \brief Way for derived class to remove value from container.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::remove_value(const value_type& val)
	{
		values_map_.erase(val);
	}

	/// \brief Function used for deeper static analysis. It tries to evaluates if expression
	/// will be at least once true and body will be executed. 
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE bool control_flow_search<Derived, Value, PrototypePolicy>::should_traverse_body(clang::Expr* expr) const
	{
		BOBOPT_UNUSED_EXPRESSION(expr);
		return false;
	}

	/// \brief Apppend values from instance to this visitor instance.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::append_values(const instance_type& visitor)
	{
		using namespace std;

		if (visitor.valid())
		{
			const container_type& visitor_map = static_cast<const base_type&>(visitor.get()).values_map_;

			for (const auto& it : visitor_map)
			{
				locations_type& locations = values_map_[it.first];
				const locations_type& visitor_locations = it.second;

				locations.reserve(locations.size() + visitor_locations.size());
				copy(begin(visitor_locations), end(visitor_locations), back_inserter(locations));
			}
		}
	}

	/// \brief Append values from container 
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::append_values(const values_type& values)
	{
		for (const auto& val : values)
		{
			values_map_.insert(std::make_pair(val, locations_type()));
		}
	}

	/// \brief Append values from container to this visitor instance and copy their locations from visitor instance.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::append_values_locations(const values_type& values, const instance_type& visitor)
	{
		if (visitor.valid())
		{
			const container_type& visitor_map = static_cast<const base_type&>(visitor.get()).values_map_;

			for (const auto& val : values)
			{
				locations_type& locations = values_map_[val];

				auto found = visitor_map.find(val);
				if (found != end(visitor_map))
				{
					const locations_type& visitor_locations = found->second;
					locations.reserve(locations.size() + visitor_locations.size());
					copy(begin(visitor_locations), end(visitor_locations), back_inserter(locations));
				}
			}
		}
	}

	/// \brief Make values in container unique.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE void control_flow_search<Derived, Value, PrototypePolicy>::make_unique(values_type& values)
	{
		using namespace std;

		sort(begin(values), end(values));

		auto unique_end = unique(begin(values), end(values));
		values.erase(unique_end, end(values));
	}

	/// \brief Create container with intersection of values from other containers.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::values_type control_flow_search<Derived, Value, PrototypePolicy>::make_intersection(values_type lhs, values_type rhs)
	{
		make_unique(lhs);
		make_unique(rhs);

		values_type result;
		result.reserve(lhs.size() + rhs.size());

		set_intersection(begin(lhs), end(lhs), begin(rhs), end(rhs), back_inserter(result));

		return result;
	}

	/// \brief Create container with union of values from other containers.
	template<typename Derived, typename Value, template <typename> class PrototypePolicy>
	BOBOPT_INLINE typename control_flow_search<Derived, Value, PrototypePolicy>::values_type control_flow_search<Derived, Value, PrototypePolicy>::make_union(values_type lhs, const values_type& rhs)
	{
		using namespace std;

		lhs.reserve(lhs.size() + rhs.size());
		copy(begin(rhs), end(rhs), back_inserter(lhs));
		make_unique(lhs);
		
		return lhs;
	}

} // namespace

#endif // guard
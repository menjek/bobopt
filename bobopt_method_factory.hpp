/// \file bobopt_method_factory.hpp Contains definition of factory methods and their mapping to method_type enumeration.

#ifndef BOBOPT_METHOD_FACTORY_HPP_GUARD_
#define BOBOPT_METHOD_FACTORY_HPP_GUARD_

namespace bobopt {

	class basic_method;

	/// \brief Optimization methods supported by optimizer.
	///
	/// Every method has its own class that handles optimizations over
	/// bobox box represented by part of AST tree. Its root is CXXRecordDecl
	/// AST node.
	enum method_type
	{
		OM_PREFETCH = 0,
		OM_YIELD_COMPLEX = 1,

		OM_COUNT
	};

	// Factory functions declarations.
	typedef basic_method* (*method_factory_function)();

	basic_method* create_prefetch();
	basic_method* create_yield_complex();

	/// \brief Class that handles mapping factory methods to enumeration type.
	///
	/// It is responsible for creation of all optimization methods objects.
	class method_factory
	{
	public:
		static basic_method* create(method_type method);

	private:
		static method_factory_function factories_[OM_COUNT];
	};

} // namespace

#endif // guard
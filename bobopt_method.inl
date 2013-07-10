namespace bobopt {

	BOBOPT_INLINE void basic_method::set_ast_context(clang::ASTContext* context)
	{
		context_ = context;
	}

	BOBOPT_INLINE clang::ASTContext* basic_method::get_ast_context() const
	{
		return context_;
	}

} // namespace
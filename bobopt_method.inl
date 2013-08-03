namespace bobopt {

	BOBOPT_INLINE void basic_method::set_compiler(clang::CompilerInstance* compiler)
	{
		compiler_ = compiler;
	}

	BOBOPT_INLINE clang::CompilerInstance* basic_method::get_compiler() const
	{
		return compiler_;
	}

} // namespace
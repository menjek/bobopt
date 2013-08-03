namespace bobopt {

	BOBOPT_INLINE void basic_method::set_optimizer(const optimizer* optimizer_instance)
	{
		optimizer_ = optimizer_instance;
	}

	BOBOPT_INLINE const optimizer* basic_method::get_optimizer() const
	{
		return optimizer_;
	}

} // namespace
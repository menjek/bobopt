namespace bobopt
{

    BOBOPT_INLINE const optimizer& basic_method::get_optimizer() const
    {
        return *optimizer_;
    }

} // namespace
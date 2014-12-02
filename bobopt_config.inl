namespace bobopt
{

    // config_map:
    //==========================================================================

    /// \brief Singleton access point.
    BOBOPT_INLINE config_map& config_map::instance()
    {
        static config_map instance;
        return instance;
    }

    /// \brief Group registration called from config_group constructor.
    BOBOPT_INLINE bool config_map::add(config_group* group)
    {
        return groups_.insert(std::make_pair(group->get_name(), group)).second;
    }

    /// \brief Access to group information by name.
    BOBOPT_INLINE config_group* config_map::get_group(const std::string& name) const
    {
        auto found = groups_.find(name);
        if (found == std::end(groups_))
        {
            return nullptr;
        }
        return found->second;
    }

    /// \brief Begin iterator to group map.
    BOBOPT_INLINE config_map::group_iterator config_map::groups_begin() const
    {
        return std::begin(groups_);
    }

    /// \brief End iterator to group map.
    BOBOPT_INLINE config_map::group_iterator config_map::groups_end() const
    {
        return std::end(groups_);
    }

    // config_group:
    //==========================================================================

    /// \brief Register in configuration map.
    BOBOPT_INLINE config_group::config_group(std::string name)
        : name_(std::move(name))
    {
        BOBOPT_CHECK(config_map::instance().add(this));
    }

    /// \brief Used for lookup in configuration map.
    BOBOPT_INLINE std::string config_group::get_name() const
    {
        return name_;
    }

    /// \brief Access configuration variable.
    BOBOPT_INLINE basic_config_variable& config_group::get_variable(const std::string& name)
    {
        BOBOPT_ASSERT(variables_.find(name) != std::end(variables_));
        return *(variables_[name]);
    }

    /// \brief Add configuration variable to the group.
    BOBOPT_INLINE bool config_group::add(basic_config_variable* variable)
    {
        return variables_.insert(std::make_pair(variable->get_name(), variable)).second;
    }

    /// \brief Begin iterator to variables.
    BOBOPT_INLINE config_group::variable_iterator config_group::variables_begin() const
    {
        return std::begin(variables_);
    }

    /// \brief End iterator to variables.
    BOBOPT_INLINE config_group::variable_iterator config_group::variables_end() const
    {
        return std::end(variables_);
    }

} // namespace
/// \file bobopt_config.hpp File contains definition of configuration facilities.

#ifndef BOBOPT_CONFIG_HPP_GUARD_
#define BOBOPT_CONFIG_HPP_GUARD_

#include <bobopt_debug.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_parser.hpp>

#include <map>
#include <regex>
#include <string>
#include <utility>

namespace bobopt
{
    // forward declaration:
    class config_group;

    /// \brief Gateway class to all configurable groups and variables.
    ///
    /// It uses Meyers singleton for access by groups for registration.
    /// Drawbacks of singleton are not visible in optimizer initialization part
    /// since its running in single-threaded environment. Later in runtime,
    /// data structures are read-only.
    class config_map
    {
    public:

        /// \brief Meyers singleton access point.
        static config_map& instance()
        {
            static config_map obj;
            return obj;
        }

        /// \brief Registration function for groups.
        bool add(config_group* group);

        /// \brief Access to the group information.
        config_group* get_group(const std::string& name) const
        {
            auto found = groups_.find(name);
            if (found == std::end(groups_))
            {
                return nullptr;
            }
            return found->second;
        }

    private:
        std::map<std::string, config_group*> groups_;
    };

    /// \brief Base class/interface for all configuration variables.
    ///
    /// Those variables are templates so they need base for storage at least.
    class basic_config_variable
    {
    public:

        /// \brief Get name of the variable
        virtual std::string get_name() const = 0;
        /// \brief Set variable value from pure text.
        virtual void set(const std::string& text) = 0;
        /// \brief Return default value as text.
        virtual std::string default_value() const = 0;
    };

    /// \brief Configuration group to keep variables.
    class config_group
    {
    public:

        /// \brief Register in configuration map.
        config_group(std::string name) : name_(std::move(name))
        {
            BOBOPT_CHECK(config_map::instance().add(this));
        }

        /// \brief Used for lookup in configuration map.
        std::string get_name() const
        {
            return name_;
        }

        /// \brief Access configuration variable.
        basic_config_variable& get_variable(const std::string& name)
        {
            BOBOPT_ASSERT(variables_.find(name) != std::end(variables_));
            return *(variables_[name]);
        }

        /// \brief Add configuration variable to the group.
        bool add(basic_config_variable* variable)
        {
            return variables_.emplace(variable->get_name(), variable).second;
        }

    private:
        std::string name_;
        std::map<std::string, basic_config_variable*> variables_;
    };

    /// \brief Configuration variable templated by type and parser.
    template <typename ValueT, typename ParserT = parser<ValueT> >
    class config_variable : public basic_config_variable
    {
    public:

        /// \brief Register variable in selected group.
        config_variable(config_group& group, std::string name, ValueT def_value)
            : name_(std::move(name))
            , value_(def_value)
            , default_value_(def_value)
            , parser_()
        {
            BOBOPT_CHECK(group.add(this));
        }

        virtual std::string get_name() const override
        {
            return name_;
        }

        virtual void set(const std::string& text) override
        {
            value_ = parser_.parse(text);
        }

        virtual std::string default_value() const override
        {
            return parser_.print(default_value_);
        }

        ValueT get() const
        {
            return value_;
        }

    private:
        BOBOPT_NONCOPYMOVABLE(config_variable);

        std::string name_;

        ValueT value_;
        ValueT const default_value_;

        ParserT parser_;
    };

    /// \brief Helper used to parse configuration file.
    class config_parser
    {
    public:
        config_parser() : group_(nullptr)
        {
        }

        bool load(const std::string& file_name);
        bool save(const std::string& file_name) const;

    private:
        bool parse_line(const std::string& line);

        config_group* group_;

        static const std::regex REGEX_GROUP;
        static const std::regex REGEX_VARIABLE;

        static const std::regex REGEX_COMMENT;
        static const std::regex REGEX_EMPTY_LINE;
    };

} // bobopt

#endif // guard

/// \file bobopt_config.hpp File contains definition of tool configuration
/// facilities.

#ifndef BOBOPT_CONFIG_HPP_GUARD_
#define BOBOPT_CONFIG_HPP_GUARD_

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_parser.hpp>

#include <map>
#include <regex>
#include <string>
#include <utility>

namespace bobopt
{
    class config_group;

    /// \brief Gateway singleton to all configurable groups and variables.
    class config_map
    {
        typedef std::map<std::string, config_group*> groups_type;

    public:
        /// \brief Singleton access point.
        static config_map& instance();

        /// \brief Group registration called from config_group constructor.
        bool add(config_group* group);
        /// \brief Access to group information by name.
        config_group* get_group(const std::string& name) const;

        /// \brief Iterator type to group map: pair<name, group>.
        typedef groups_type::const_iterator group_iterator;
        /// \brief Begin iterator to group map.
        group_iterator groups_begin() const;
        /// \brief End iterator to group map.
        group_iterator groups_end() const;

    private:
        groups_type groups_;
    };

    /// \brief Base class/interface for all configuration variables.
    class basic_config_variable
    {
    public:
        /// \brief Get name of the variable.
        virtual std::string get_name() const = 0;
        /// \brief Set variable value from text.
        virtual void set(const std::string& text) = 0;
        /// \brief Return default variable value as a text.
        virtual std::string default_value() const = 0;
    };

    /// \brief Configuration group for variables.
    class config_group
    {
        typedef std::map<std::string, basic_config_variable*> variables_type;

    public:
        /// \brief Register in configuration map.
        explicit config_group(std::string name);

        /// \brief Used for lookup in configuration map.
        std::string get_name() const;

        /// \brief Access configuration variable.
        basic_config_variable& get_variable(const std::string& name);

        /// \brief Add configuration variable to the group.
        bool add(basic_config_variable* variable);

        /// \brief Iterator type for variables: pair<name, variable>.
        typedef variables_type::const_iterator variable_iterator;
        /// \brief Begin iterator to variables.
        variable_iterator variables_begin() const;
        /// \brief End iterator to variables.
        variable_iterator variables_end() const;

    private:
        std::string name_;
        variables_type variables_;
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

        // inherited members:
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

        /// \brief Access value of configuration variable.
        BOBOPT_INLINE ValueT get() const
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

    /// \brief Helper for save/load of configuration file.
    class config_parser
    {
    public:
        config_parser()
            : group_(nullptr)
        {
        }

        /// \brief Load configuration from specific file.
        bool load(const std::string& file_name);
        /// \brief Save configuration to specific file.
        bool save(const std::string& file_name) const;

    private:
        /// \brief Helper to parse single line of configuration file.
        bool parse_line(const std::string& line);

        config_group* group_;

        // constants:
        static const std::regex REGEX_GROUP;
        static const std::regex REGEX_VARIABLE;
        static const std::regex REGEX_COMMENT;
        static const std::regex REGEX_EMPTY_LINE;
    };

} // bobopt

#include BOBOPT_INLINE_IN_HEADER(bobopt_config.inl)

#endif // guard

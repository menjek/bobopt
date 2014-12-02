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

    // config_map:
    //==========================================================================

    /// \brief Gateway singleton to all configurable groups and variables.
    class config_map
    {
        typedef std::map<std::string, config_group*> groups_type;

    public:
        static config_map& instance();

        bool add(config_group* group);
        config_group* get_group(const std::string& name) const;

        typedef groups_type::const_iterator group_iterator;
        group_iterator groups_begin() const;
        group_iterator groups_end() const;

    private:
        groups_type groups_;
    };

    // basic_config_variable:
    //==========================================================================

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

    // config_group:
    //==========================================================================

    /// \brief Configuration group for variables.
    class config_group
    {
        typedef std::map<std::string, basic_config_variable*> variables_type;

    public:
        explicit config_group(std::string name);

        std::string get_name() const;
        basic_config_variable& get_variable(const std::string& name);
        bool add(basic_config_variable* variable);

        typedef variables_type::const_iterator variable_iterator;
        variable_iterator variables_begin() const;
        variable_iterator variables_end() const;

    private:
        std::string name_;
        variables_type variables_;
    };

    // config_variable<>:
    //==========================================================================

    /// \brief Configuration variable templated by type and parser.
    template <typename ValueT, typename ParserT = parser<ValueT> >
    class config_variable : public basic_config_variable
    {
    public:
        /// \brief Register variable in selected group.
        config_variable(config_group& group, std::string name, const ValueT& def_value)
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
        const ValueT default_value_;

        ParserT parser_;
    };

    // config_parser:
    //==========================================================================

    /// \brief Helper for save/load of configuration file.
    class config_parser
    {
    public:
        config_parser()
            : group_(nullptr)
        {
        }

        bool load(const std::string& file_name);
        bool save(const std::string& file_name) const;

    private:
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

/// \file bobopt_parser.hpp File contains definition of parser for basic types.
#ifndef BOBOPT_PARSER_HPP_GUARD_
#define BOBOPT_PARSER_HPP_GUARD_

#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>

#include <stdexcept>
#include <string>

namespace bobopt
{

    /// \brief Undefined for all types.
    template <typename T>
    struct parser;

    /// \brief Specialization for bool.
    ///
    /// Allowed values: true, TRUE, True, 1, resp. false, FALSE, False, 0.
    template <>
    struct parser<bool>
    {
        BOBOPT_INLINE bool parse(const std::string& text)
        {
            if ((text == "true") || (text == "TRUE") || (text == "True") || (text == "1"))
            {
                return true;
            }

            if ((text == "false") || (text == "FALSE") || (text == "False") || (text == "0"))
            {
                return false;
            }

            throw std::invalid_argument(text + " is not boolean");
        }

        BOBOPT_INLINE std::string print(bool value) const
        {
            return value ? "true" : "false";
        }
    };

    /// \brief Specialization for unsigned integers.
    template <>
    struct parser<unsigned>
    {
        BOBOPT_INLINE unsigned parse(const std::string& text)
        {
            return std::stoul(text);
        }

        BOBOPT_INLINE std::string print(unsigned value) const
        {
            return std::to_string(value);
        }
    };

    /// \brief Specialization for unsigned long integer.
    template <>
    struct parser<unsigned long>
    {
        BOBOPT_INLINE unsigned long parse(const std::string& text)
        {
            return std::stoul(text);
        }

        BOBOPT_INLINE std::string print(unsigned long value) const
        {
            return std::to_string(value);
        }
    };

    /// \brief Specialization for unsigned long long integers.
    template <>
    struct parser<unsigned long long>
    {
        BOBOPT_INLINE unsigned long long parse(const std::string& text)
        {
            return std::stoull(text);
        }

        BOBOPT_INLINE std::string print(unsigned long long value) const
        {
            return std::to_string(value);
        }
    };

    /// \brief Specialization for signed integers.
    template <>
    struct parser<int>
    {
        BOBOPT_INLINE int parse(const std::string& text)
        {
            return std::stoi(text);
        }

        BOBOPT_INLINE std::string print(int value) const
        {
            return std::to_string(value);
        }
    };

    /// \brief Specialization for signed long integers.
    template <>
    struct parser<long>
    {
        BOBOPT_INLINE long parse(const std::string& text)
        {
            return std::stol(text);
        }

        BOBOPT_INLINE std::string print(long value) const
        {
            return std::to_string(value);
        }
    };

    /// \brief Specialization for signed long long integers.
    template <>
    struct parser<long long>
    {
        BOBOPT_INLINE long long parse(const std::string& text)
        {
            return std::stoll(text);
        }

        BOBOPT_INLINE std::string print(long long value) const
        {
            return std::to_string(value);
        }
    };

} // bobopt

#endif // guard
#include <bobopt_config.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/raw_ostream.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <fstream>

#include BOBOPT_INLINE_IN_SOURCE(bobopt_config.inl)

namespace bobopt
{

    // Constants.
    //==========================================================================

    /// \brief Regular expression for line defining group.
    const std::regex config_parser::REGEX_GROUP(R"(\[([a-zA-Z0-9_ ]+)\])");
    /// \brief Regular expression for line defining variable.
    const std::regex config_parser::REGEX_VARIABLE(R"(([a-zA-Z0-9_]+)\s*:\s*(.*))");
    /// \brief Regular expression for line with comment.
    const std::regex config_parser::REGEX_COMMENT(R"(\s*#.*)");
    /// \brief Regular expression for empty line.
    const std::regex config_parser::REGEX_EMPTY_LINE(R"(\s*)");

    // Implementation.
    //==========================================================================

    /// \brief Load configuration from specific file.
    bool config_parser::load(const std::string& file_name)
    {
        std::ifstream file(file_name);
        if (!file)
        {
            return false;
        }

        try
        {
            for (std::string line; std::getline(file, line);)
            {
                if (!parse_line(line))
                {
                    return false;
                }
            }
        }
        catch (const std::exception& e)
        {
            llvm::errs() << "Error: " << e.what() << '\n';
            return false;
        }

        return true;
    }

    /// \brief Save configuration to specific file.
    bool config_parser::save(const std::string& file_name) const
    {
        std::ofstream file(file_name);
        if (!file)
        {
            return false;
        }

        config_map& cfg = config_map::instance();
        for (auto it = cfg.groups_begin(), end = cfg.groups_end(); it != end; ++it)
        {
            file << '[' << it->first << ']' << std::endl;
            file << std::endl;

            auto* group = it->second;
            for (auto vit = group->variables_begin(), vend = group->variables_end(); vit != vend; ++vit)
            {
                file << vit->first << ": " << vit->second->default_value() << std::endl;
            }

            file << std::endl;
        }

        return true;
    }

    /// \brief Helper to parse single line of configuration file.
    bool config_parser::parse_line(const std::string& line)
    {
        // Variable line should be the most frequent.
        {
            std::smatch m;
            if (std::regex_match(line, m, REGEX_VARIABLE))
            {
                if (group_ == nullptr)
                {
                    return true;
                }

                auto& variable = group_->get_variable(m[1].str());
                variable.set(m[2].str());
                return true;
            }
        }

        // Empty line should be the second most frequent
        {
            if (std::regex_match(line, REGEX_EMPTY_LINE))
            {
                return true;
            }
        }

        // Group line the next most frequent.
        {
            std::smatch m;
            if (std::regex_match(line, m, REGEX_GROUP))
            {
                group_ = config_map::instance().get_group(m[1].str());
                return true;
            }
        }

        // The last option is comment, otherwise misformed config file.
        return std::regex_match(line, REGEX_COMMENT);
    }

} // bobopt

namespace bobopt
{

    // source_message:
    //==========================================================================

    /// \brief Construct diagnostic message.
    BOBOPT_INLINE
    diagnostic_message::diagnostic_message(types type, clang::SourceRange range, clang::SourceRange point_range, const std::string& message)
        : type_(type)
        , range_(range)
        , point_range_(point_range)
        , message_(message)
    {
    }

    /// \brief Get type of diagnostic message.
    BOBOPT_INLINE diagnostic_message::types diagnostic_message::get_type() const
    {
        return type_;
    }

    /// \brief Get source location range for diagnostic message.
    BOBOPT_INLINE clang::SourceRange diagnostic_message::get_range() const
    {
        return range_;
    }

    /// \brief Get range of pointers to source code.
    BOBOPT_INLINE clang::SourceRange diagnostic_message::get_point_range() const
    {
        return point_range_;
    }

    /// brief Get user message itself.
    BOBOPT_INLINE std::string diagnostic_message::get_message() const
    {
        return message_;
    }

    // diagnostic:
    //==========================================================================

    /// \brief Construct diagnostic with reference to compiler.
    BOBOPT_INLINE diagnostic::diagnostic(clang::CompilerInstance& compiler)
        : compiler_(compiler)
    {
    }

} // namespace
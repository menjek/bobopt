namespace bobopt
{

    // source_message inlines.
    //==============================================================================

    BOBOPT_INLINE source_message::source_message(types type, clang::SourceRange range, clang::SourceRange point_range, const std::string& message)
        : type_(type)
        , range_(range)
        , point_range_(point_range)
        , message_(message)
    {
    }

    BOBOPT_INLINE source_message::types source_message::get_type() const
    {
        return type_;
    }

    BOBOPT_INLINE clang::SourceRange source_message::get_range() const
    {
        return range_;
    }

    BOBOPT_INLINE clang::SourceRange source_message::get_point_range() const
    {
        return point_range_;
    }

    BOBOPT_INLINE std::string source_message::get_message() const
    {
        return message_;
    }

    // diagnostic inlines.
    //==============================================================================

    BOBOPT_INLINE diagnostic::diagnostic(clang::CompilerInstance& compiler) : compiler_(compiler)
    {
    }

} // namespace
project(bobopt)
set(CMAKE_SUPPRESS_REGENERATION true)

set(bobopt_SOURCES
  clang/bobopt_clang_utils.cpp
  clang/bobopt_clang_epilog.hpp
  clang/bobopt_clang_prolog.hpp
  clang/bobopt_clang_utils.hpp
  clang/bobopt_control_flow_search.hpp
  clang/bobopt_clang_utils.inl
  methods/bobopt_prefetch.cpp
  methods/bobopt_yield_complex.cpp
  methods/bobopt_prefetch.hpp
  methods/bobopt_yield_complex.hpp
  bobopt_config.cpp
  bobopt_diagnostic.cpp
  bobopt_method.cpp
  bobopt_method_factory.cpp
  bobopt_optimizer.cpp
  bobopt_text_utils.cpp
  main.cpp
  bobopt_config.hpp
  bobopt_debug.hpp
  bobopt_diagnostic.hpp
  bobopt_inline.hpp
  bobopt_language.hpp
  bobopt_macros.hpp
  bobopt_method.hpp
  bobopt_method_factory.hpp
  bobopt_optimizer.hpp
  bobopt_parser.hpp
  bobopt_text_utils.hpp
  bobopt_utils.hpp
  bobopt_diagnostic.inl
  bobopt_empty.inl
  bobopt_method.inl
  bobopt_optimizer.inl
  bobopt_text_utils.inl
  )
  
add_executable(bobopt ${bobopt_SOURCES})
#include <bobopt_debug.hpp>
#include <bobopt_method_factory.hpp>

namespace bobopt
{

    method_factory_function method_factory::factories_[OM_COUNT] = { create_prefetch,     // OM_PREFETCH
                                                                     create_yield_complex // OM_YIELD_COMPLEX
    };

    basic_method* method_factory::create(method_type method)
    {
        BOBOPT_ASSERT(method < OM_COUNT);
        basic_method* result = (factories_[method])();
        BOBOPT_ASSERT(result != nullptr);

        return result;
    }

} // namespace
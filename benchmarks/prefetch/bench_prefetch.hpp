#ifndef BOBOPT_BENCHMARKS_PREFETCH_BENCH_PREFETCH_HPP_GUARD_
#define BOBOPT_BENCHMARKS_PREFETCH_BENCH_PREFETCH_HPP_GUARD_

#include <benchmarks/bobox_prolog.hpp>
#include <bobox_basic_box.hpp>
#include <benchmarks/bobox_epilog.hpp>

namespace bobopt
{

    class test_box : public bobox::basic_box
    {
    public:
        typedef generic_model<test_box, bobox::BST_STATEFUL> model;

        test_box(const box_parameters_pack& box_params)
            : bobox::basic_box(box_params)
        {
        }
    };

} // bobopt

#endif // guard
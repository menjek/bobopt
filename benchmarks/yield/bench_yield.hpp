#ifndef BOBOPT_BENCHMARKS_PREFETCH_BENCH_YIELD_HPP_GUARD_
#define BOBOPT_BENCHMARKS_PREFETCH_BENCH_YIELD_HPP_GUARD_

#include <benchmarks/bench_utils.hpp>

#include <benchmarks/bobox_prolog.hpp>
#include <bobox_basic_box.hpp>
#include <bobox_basic_box_utils.hpp>

#include <bobopt_macros.hpp>

namespace bobopt
{
    static const unsigned TEST_SIZE = 10;

    class source_box : public bobox::basic_box
    {
    public:
        typedef generic_model<source_box, bobox::BST_STATEFUL> model;

        BOBOX_BOX_INPUTS_LIST(main, 0);
        BOBOX_BOX_OUTPUTS_LIST(out0, 0, out1, 1, out2, 2, out3, 3, out4, 4, out5, 5, out6, 6, out7, 7, end, 8);

        source_box(const box_parameters_pack& box_params)
            : bobox::basic_box(box_params)
        {
        }

        virtual void init_impl() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;
            prefetch_envelope(inputs::main());
        }

        virtual void sync_body() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;

            BOBOX_ASSERT(pop_envelope(inputs::main())->is_poisoned());

            for (unsigned i = 0u; i < TEST_SIZE; ++i)
            {
                BENCH_LOG_MEMFUNC_MSG("Started to work.");

                // One of two sequential tasks in work_box. 5s of work.
                do_hard_work();
                do_hard_work();
                do_hard_work();
                do_hard_work();
                do_hard_work();

                BENCH_LOG_MEMFUNC_MSG("Finished work => sending data => yield.");

                // Unleash the kraken.
                bench_send_envelope(this, outputs::out0(), i);
                bench_send_envelope(this, outputs::out1(), i);
                bench_send_envelope(this, outputs::out2(), i);
                bench_send_envelope(this, outputs::out3(), i);
                bench_send_envelope(this, outputs::out4(), i);
                bench_send_envelope(this, outputs::out5(), i);
                bench_send_envelope(this, outputs::out6(), i);
                bench_send_envelope(this, outputs::out7(), i);

                do_little_work();

                // let others calculate.
                yield();
            }

            // Finish job.
            send_poisoned(outputs::out0());
            send_poisoned(outputs::out1());
            send_poisoned(outputs::out2());
            send_poisoned(outputs::out3());
            send_poisoned(outputs::out4());
            send_poisoned(outputs::out5());
            send_poisoned(outputs::out6());
            send_poisoned(outputs::out7());
            send_poisoned(outputs::end());
        }
    };

    class work_box : public bobox::basic_box
    {
    public:
        typedef generic_model<work_box, bobox::BST_STATELESS> model;

        BOBOX_BOX_INPUTS_LIST(main, 0);
        BOBOX_BOX_OUTPUTS_LIST(main, 0);

        work_box(const box_parameters_pack& box_params)
            : bobox::basic_box(box_params)
        {
        }

        virtual void init_impl() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;
            prefetch_envelope(inputs::main());
        }

        virtual void sync_body() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;

            auto env = pop_envelope(inputs::main());
            BOBOPT_UNUSED_EXPRESSION(env);

            if (env->is_poisoned())
            {
                return;
            }

            // Simulate hard work for the first sequential task.
            for (unsigned int i = 0; i < 1; ++i)
            {
                for (unsigned int j = 0; j < 1; ++j)
                {
                    BENCH_LOG_MEMFUNC_MSG("Started to work [1st task].");

                    // 5s of work
                    do_hard_work();
                    do_hard_work();
                    do_hard_work();
                    do_hard_work();
                    do_hard_work();
                }
            }

            // yield should be placed in between

            // Simulate hard work for the second sequential task.
            for (unsigned int i = 0; i < 1; ++i)
            {
                for (unsigned int j = 0; j < 1; ++j)
                {
                    BENCH_LOG_MEMFUNC_MSG("Started to work [2nd task].");

                    // 5s of work
                    do_hard_work();
                    do_hard_work();
                    do_hard_work();
                    do_hard_work();
                    do_hard_work();
                }
            }
        }
    };

} // bobopt

#include <benchmarks/bobox_epilog.hpp>

#endif // guard
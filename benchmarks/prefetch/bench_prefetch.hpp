#ifndef BOBOPT_BENCHMARKS_PREFETCH_BENCH_PREFETCH_HPP_GUARD_
#define BOBOPT_BENCHMARKS_PREFETCH_BENCH_PREFETCH_HPP_GUARD_

#include <benchmarks/bench_utils.hpp>

#include <benchmarks/bobox_prolog.hpp>
#include <bobox_basic_box.hpp>
#include <bobox_basic_box_utils.hpp>

namespace bobopt
{
    static const unsigned TEST_SIZE = 1000000u;

    class control_box : public bobox::basic_box
    {
    public:
        typedef generic_model<control_box, bobox::BST_STATEFUL> model;

        BOBOX_BOX_INPUTS_LIST(main, 0);
        BOBOX_BOX_OUTPUTS_LIST(main, 0);

        control_box(const box_parameters_pack& box_params)
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

            for (unsigned i = 0u; i <= TEST_SIZE; ++i)
            {
                bench_send_envelope(this, outputs::main(), i);
            }

            send_poisoned(outputs::main());
        }
    };

    class distribute_box : public bobox::basic_box
    {
    public:
        typedef generic_model<distribute_box, bobox::BST_STATELESS> model;

        BOBOX_BOX_INPUTS_LIST(main, 0);
        BOBOX_BOX_OUTPUTS_LIST(out0, 0, out1, 1, out2, 2, out3, 3, out4, 4, out5, 5, out6, 6, out7, 7, next, 8);

        distribute_box(const box_parameters_pack& box_params)
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
            auto env_in = pop_envelope(inputs::main());
            if (env_in->is_poisoned())
            {
                send_poisoned(outputs::out0());
                send_poisoned(outputs::out1());
                send_poisoned(outputs::out2());
                send_poisoned(outputs::out3());
                send_poisoned(outputs::out4());
                send_poisoned(outputs::out5());
                send_poisoned(outputs::out6());
                send_poisoned(outputs::out7());
                send_poisoned(outputs::next());
                return;
            }

            auto in = *(env_in->get_column(column_index_type(0)).get_data<unsigned>());

            BENCH_LOG_MEMFUNC_MSG("distributing data.");

            bench_send_envelope(this, outputs::out0(), in);
            bench_send_envelope(this, outputs::out1(), in);
            bench_send_envelope(this, outputs::out2(), in);
            bench_send_envelope(this, outputs::out3(), in);
            bench_send_envelope(this, outputs::out4(), in);
            bench_send_envelope(this, outputs::out5(), in);
            bench_send_envelope(this, outputs::out6(), in);
            bench_send_envelope(this, outputs::out7(), in);
            bench_send_envelope(this, outputs::next(), in);
        }
    };

    class last_distribute_box : public bobox::basic_box
    {
    public:
        typedef generic_model<last_distribute_box, bobox::BST_STATELESS> model;

        BOBOX_BOX_INPUTS_LIST(main, 0);
        BOBOX_BOX_OUTPUTS_LIST(out0, 0, out1, 1, out2, 2, out3, 3, out4, 4, out5, 5, out6, 6, out7, 7);

        last_distribute_box(const box_parameters_pack& box_params)
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
            auto env_in = pop_envelope(inputs::main());
            if (env_in->is_poisoned())
            {
                send_poisoned(outputs::out0());
                send_poisoned(outputs::out1());
                send_poisoned(outputs::out2());
                send_poisoned(outputs::out3());
                send_poisoned(outputs::out4());
                send_poisoned(outputs::out5());
                send_poisoned(outputs::out6());
                send_poisoned(outputs::out7());
                return;
            }

            auto in = *(env_in->get_column(column_index_type(0)).get_data<unsigned>());

            BENCH_LOG_MEMFUNC_MSG("the last data distribution.");

            bench_send_envelope(this, outputs::out0(), in);
            bench_send_envelope(this, outputs::out1(), in);
            bench_send_envelope(this, outputs::out2(), in);
            bench_send_envelope(this, outputs::out3(), in);
            bench_send_envelope(this, outputs::out4(), in);
            bench_send_envelope(this, outputs::out5(), in);
            bench_send_envelope(this, outputs::out6(), in);
            bench_send_envelope(this, outputs::out7(), in);
        }
    };

    class collect_box : public bobox::basic_box
    {
    public:
        typedef generic_model<collect_box, bobox::BST_STATELESS> model;

        BOBOX_BOX_INPUTS_LIST(in0, 0, in1, 1, in2, 2, in3, 3, in4, 4, in5, 5, in6, 6, in7, 7);
        BOBOX_BOX_OUTPUTS_LIST(main, 0);

        collect_box(const box_parameters_pack& box_params)
            : bobox::basic_box(box_params)
        {
        }

        virtual void init_impl() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;
            prefetch_envelope(inputs::in0());
        }

        virtual void sync_body() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;
            auto env_in0 = pop_envelope(inputs::in0());
            auto env_in1 = pop_envelope(inputs::in1());
            auto env_in2 = pop_envelope(inputs::in2());
            auto env_in3 = pop_envelope(inputs::in3());
            auto env_in4 = pop_envelope(inputs::in4());
            auto env_in5 = pop_envelope(inputs::in5());
            auto env_in6 = pop_envelope(inputs::in6());
            auto env_in7 = pop_envelope(inputs::in7());

            if (env_in0->is_poisoned() || env_in1->is_poisoned() || env_in2->is_poisoned() || env_in3->is_poisoned() || env_in4->is_poisoned() ||
                env_in5->is_poisoned() || env_in6->is_poisoned() || env_in7->is_poisoned())
            {
                send_poisoned(outputs::main());
                return;
            }

            BENCH_LOG_MEMFUNC_MSG("data collected.");

            auto in0 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in1 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in2 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in3 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in4 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in5 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in6 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());
            auto in7 = *(env_in0->get_column(column_index_type(0)).get_data<unsigned>());

            bench_send_envelope(this, outputs::main(), in0 + in1 + in2 + in3 + in4 + in5 + in6 + in7);
        }
    };

    class sink_box : public bobox::basic_box
    {
    public:
        typedef generic_model<sink_box, bobox::BST_STATELESS> model;

        BOBOX_BOX_INPUTS_LIST(in0, 0, in1, 1, in2, 2, in3, 3, in4, 4, in5, 5, in6, 6, in7, 7);
        BOBOX_BOX_OUTPUTS_LIST(main, 0);

        sink_box(const box_parameters_pack& box_params)
            : bobox::basic_box(box_params)
        {
        }

        virtual void init_impl() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;
            prefetch_envelope(inputs::in0());
        }

        virtual void sync_body() BOBOX_OVERRIDE
        {
            BENCH_LOG_MEMFUNC;

            auto env_in0 = pop_envelope(inputs::in0());
            auto env_in1 = pop_envelope(inputs::in1());
            auto env_in2 = pop_envelope(inputs::in2());
            auto env_in3 = pop_envelope(inputs::in3());
            auto env_in4 = pop_envelope(inputs::in4());
            auto env_in5 = pop_envelope(inputs::in5());
            auto env_in6 = pop_envelope(inputs::in6());
            auto env_in7 = pop_envelope(inputs::in7());

            BENCH_LOG_MEMFUNC_MSG("data in the sink.");

            if (env_in0->is_poisoned() || env_in1->is_poisoned() || env_in2->is_poisoned() || env_in3->is_poisoned() || env_in4->is_poisoned() ||
                env_in5->is_poisoned() || env_in6->is_poisoned() || env_in7->is_poisoned())
            {
                send_poisoned(outputs::main());
                return;
            }
        }
    };

} // bobopt

#include <benchmarks/bobox_epilog.hpp>

#endif // guard
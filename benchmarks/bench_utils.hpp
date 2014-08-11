#ifndef BOBOPT_BENCHMARKS_BENCH_UTILS_HPP_GUARD_
#define BOBOPT_BENCHMARKS_BENCH_UTILS_HPP_GUARD_

#include <benchmarks/bobox_prolog.hpp>
#include <bobox_basic_box.hpp>
#include <bobox_column.hpp>
#include <bobox_envelope.hpp>
#include <bobox_types.hpp>

#include <ctime>

namespace bobopt
{

    void do_work(const std::clock_t ticks);

    void do_little_work();
    void do_some_work();
    void do_hard_work();

    template <typename T>
    bobox::envelope_ptr_type bench_make_envelope(bobox::basic_box* box, bobox::output_index_type out, T value)
    {
        bobox::envelope* envelope = box->allocate(box->get_output_descriptor(out), 1);
        envelope->set_size(1);
        auto* data = envelope->get_column(bobox::column_index_type(0)).get_data<T>();
        *data = value;
        return bobox::envelope_ptr_type(envelope);
    }

    template <typename T>
    void bench_send_envelope(bobox::basic_box* box, bobox::output_index_type out, T value)
    {
        box->send_envelope(out, bench_make_envelope(box, out, value));
    }

} // bobopt

#include <benchmarks/bobox_epilog.hpp>

#endif // guard

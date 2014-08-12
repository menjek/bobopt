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
    
    //
    // Benchmarks
    //

    /// \brief Do work for some specific number of ticks.
    /// \param tick Number of ticks.
    void do_work(const std::clock_t ticks);

    /// \brief Do work for 100 ticks.
    void do_little_work();
    /// \brief Do work for 5000 ticks.
    void do_some_work();
    /// \brief Do work for 1 second (CLOCKS_PER_SEC ticks).
    void do_hard_work();

    //
    // Box helpers.
    //

    /// \brief Create envelope on specific box and specific output with size 1 and value.
    template <typename T>
    bobox::envelope_ptr_type bench_make_envelope(bobox::basic_box* box, bobox::output_index_type out, T value)
    {
        bobox::envelope* envelope = box->allocate(box->get_output_descriptor(out), 1);
        envelope->set_size(1);
        auto* data = envelope->get_column(bobox::column_index_type(0)).get_data<T>();
        *data = value;
        return bobox::envelope_ptr_type(envelope);
    }

    /// \brief Create envelope and send. \link bench_make_envelope \endlink.
    template <typename T>
    void bench_send_envelope(bobox::basic_box* box, bobox::output_index_type out, T value)
    {
        box->send_envelope(out, bench_make_envelope(box, out, value));
    }

} // bobopt

#include <benchmarks/bobox_epilog.hpp>

#endif // guard

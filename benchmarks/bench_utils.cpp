#include <benchmarks/bench_utils.hpp>

namespace bobopt
{

    void do_work(std::clock_t ticks)
    {
        const std::clock_t end = std::clock() + ticks;
        while (std::clock() < end)
            ;
    }

    static const std::clock_t LITTLE_WORK_TICKS = 100u;
    static const std::clock_t SOME_WORK_TICKS = 5000u;
    static const std::clock_t HARD_WORK_TICKS = CLOCKS_PER_SEC;

    void do_little_work()
    {
        do_work(LITTLE_WORK_TICKS);
    }

    void do_some_work()
    {
        do_work(SOME_WORK_TICKS);
    }

    void do_hard_work()
    {
        do_work(HARD_WORK_TICKS);
    }

} // bobopt
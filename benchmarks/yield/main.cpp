#include "bench_yield.hpp"

#include <benchmarks/bobox_prolog.hpp>
#include <bobox_basic_object_factory.hpp>
#include <bobox_bobolang.hpp>
#include <bobox_manager.hpp>
#include <bobox_request.hpp>
#include <bobox_results.hpp>
#include <bobox_runtime.hpp>
#include <benchmarks/bobox_epilog.hpp>

#include <iostream>

namespace bobopt
{

    class test_runtime : public bobox::runtime, public bobox::basic_object_factory
    {
    private:
        virtual void init_impl() BOBOX_OVERRIDE
        {
            register_box<source_box::model>(bobox::box_model_tid_type("Source"));
            register_box<work_box::model>(bobox::box_model_tid_type("Work"));

            register_type<unsigned>(bobox::type_tid_type("unsigned"));
        }

        virtual bobox::runtime* get_runtime() BOBOX_OVERRIDE
        {
            return this;
        }
    };

} // bobopt

int main()
{
    auto manager_params = new bobox::basic_parameters;
    manager_params->add_parameter("SchedulingStrategy", bobox::SS_SMP);
    manager_params->add_parameter("OptimalPlevel", bobox::plevel_type(8));
    manager_params->add_parameter("BackupThreads", 0u);

    bobox::manager mng((bobox::parameters_ptr_type(manager_params)));

    bobopt::test_runtime rt;
    rt.init();

    std::string str("model main<()><()> { "
                    "	Source<()><(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),()> source; "
                    "	Work <(unsigned)><()> work0, work1, work2, work3, work4, work5, work6, work7; "
                    "	"
                    "	input -> source; "
                    "	source[0] -> work0; "
                    "	source[1] -> work1; "
                    "	source[2] -> work2; "
                    "	source[3] -> work3; "
                    "	source[4] -> work4; "
                    "	source[5] -> work5; "
                    "	source[6] -> work6; "
                    "	source[7] -> work7; "
                    "   source[8] -> output; "
                    "}");
    std::istringstream in(str);

    bobox::request_id_type rqid = mng.create_request(bobox::bobolang::compile(in, &rt));

    mng.run_request(rqid);
    mng.wait_on_request(rqid);

    switch (mng.get_result(rqid))
    {
    case bobox::RRT_ERROR:
        std::cout << "Error" << std::endl;
        break;
    case bobox::RRT_CANCELED:
        std::cout << "Canceled" << std::endl;
        break;
    case bobox::RRT_DEADLOCK:
        std::cout << "Deadlock" << std::endl;
        break;
    case bobox::RRT_MEMORY:
        std::cout << "Memory" << std::endl;
        break;
    case bobox::RRT_OK:
        std::cout << "OK" << std::endl;
        break;
    case bobox::RRT_TIMEOUT:
        std::cout << "Timeout" << std::endl;
        break;
    default:
        BOBOX_ASSERT(false);
        break;
    }

    std::cout << rqid;

    mng.destroy_request(rqid);

    return 0;
}
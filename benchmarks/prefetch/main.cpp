#include "bench_prefetch.hpp"

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
            register_box<control_box::model>(bobox::box_model_tid_type("Control"));
            register_box<distribute_box::model>(bobox::box_model_tid_type("Distribute"));
            register_box<last_distribute_box::model>(bobox::box_model_tid_type("LastDistribute"));
            register_box<collect_box::model>(bobox::box_model_tid_type("Collect"));
            register_box<sink_box::model>(bobox::box_model_tid_type("Sink"));

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
    manager_params->add_parameter("SchedulingStrategy", bobox::SS_SINGLE_THREADED);
    manager_params->add_parameter("OptimalPlevel", bobox::plevel_type(1));
    manager_params->add_parameter("BackupThreads", 0u);

    bobox::manager mng((bobox::parameters_ptr_type(manager_params)));

    bobopt::test_runtime rt;
    rt.init();

    std::string str("model main<()><()> { "
                    "	Control<()><(unsigned)> control; "
                    "	Distribute <(unsigned)><(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned)> dis0, dis1, dis2, dis3, dis4, dis5, dis6, dis7, dis8; "
                    "	LastDistribute <(unsigned)><(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned)> last_dis; "
                    "	Collect <(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned)><(unsigned)> col0, col1, col2, col3, col4, col5, col6, col7, col8, col9; "
                    "   Sink<(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned),(unsigned)><()> sink; "
                    "	"
                    "	input -> control; "
                    "	control[0] -> dis0; "
                    "	dis0[0] -> [in0]col0; "
                    "	dis0[1] -> [in0]col1; "
                    "	dis0[2] -> [in0]col2; "
                    "	dis0[3] -> [in0]col3; "
                    "	dis0[4] -> [in0]col4; "
                    "	dis0[5] -> [in0]col5; "
                    "	dis0[6] -> [in0]col6; "
                    "	dis0[7] -> [in0]col7; "
                    "	dis0[8] -> [in0]col8; "
                    "	dis0[9] -> [in0]col9; "
                    "	dis0[10] -> dis1; "
                    "	dis1[0] -> [in1]col0; "
                    "	dis1[1] -> [in1]col1; "
                    "	dis1[2] -> [in1]col2; "
                    "	dis1[3] -> [in1]col3; "
                    "	dis1[4] -> [in1]col4; "
                    "	dis1[5] -> [in1]col5; "
                    "	dis1[6] -> [in1]col6; "
                    "	dis1[7] -> [in1]col7; "
                    "	dis1[8] -> [in1]col8; "
                    "	dis1[9] -> [in1]col9; "
                    "	dis1[10] -> dis2; "
                    "	dis2[0] -> [in2]col0; "
                    "	dis2[1] -> [in2]col1; "
                    "	dis2[2] -> [in2]col2; "
                    "	dis2[3] -> [in2]col3; "
                    "	dis2[4] -> [in2]col4; "
                    "	dis2[5] -> [in2]col5; "
                    "	dis2[6] -> [in2]col6; "
                    "	dis2[7] -> [in2]col7; "
                    "	dis2[8] -> [in2]col8; "
                    "	dis2[9] -> [in2]col9; "
                    "	dis2[10] -> dis3; "
                    "	dis3[0] -> [in3]col0; "
                    "	dis3[1] -> [in3]col1; "
                    "	dis3[2] -> [in3]col2; "
                    "	dis3[3] -> [in3]col3; "
                    "	dis3[4] -> [in3]col4; "
                    "	dis3[5] -> [in3]col5; "
                    "	dis3[6] -> [in3]col6; "
                    "	dis3[7] -> [in3]col7; "
                    "	dis3[8] -> [in3]col8; "
                    "	dis3[9] -> [in3]col9; "
                    "	dis3[10] -> dis4; "
                    "	dis4[0] -> [in4]col0; "
                    "	dis4[1] -> [in4]col1; "
                    "	dis4[2] -> [in4]col2; "
                    "	dis4[3] -> [in4]col3; "
                    "	dis4[4] -> [in4]col4; "
                    "	dis4[5] -> [in4]col5; "
                    "	dis4[6] -> [in4]col6; "
                    "	dis4[7] -> [in4]col7; "
                    "	dis4[8] -> [in4]col8; "
                    "	dis4[9] -> [in4]col9; "
                    "	dis4[10] -> dis5; "
                    "	dis5[0] -> [in5]col0; "
                    "	dis5[1] -> [in5]col1; "
                    "	dis5[2] -> [in5]col2; "
                    "	dis5[3] -> [in5]col3; "
                    "	dis5[4] -> [in5]col4; "
                    "	dis5[5] -> [in5]col5; "
                    "	dis5[6] -> [in5]col6; "
                    "	dis5[7] -> [in5]col7; "
                    "	dis5[8] -> [in5]col8; "
                    "	dis5[9] -> [in5]col9; "
                    "	dis5[10] -> dis6; "
                    "	dis6[0] -> [in6]col0; "
                    "	dis6[1] -> [in6]col1; "
                    "	dis6[2] -> [in6]col2; "
                    "	dis6[3] -> [in6]col3; "
                    "	dis6[4] -> [in6]col4; "
                    "	dis6[5] -> [in6]col5; "
                    "	dis6[6] -> [in6]col6; "
                    "	dis6[7] -> [in6]col7; "
                    "	dis6[8] -> [in6]col8; "
                    "	dis6[9] -> [in6]col9; "
                    "	dis6[10] -> dis7; "
                    "	dis7[0] -> [in7]col0; "
                    "	dis7[1] -> [in7]col1; "
                    "	dis7[2] -> [in7]col2; "
                    "	dis7[3] -> [in7]col3; "
                    "	dis7[4] -> [in7]col4; "
                    "	dis7[5] -> [in7]col5; "
                    "	dis7[6] -> [in7]col6; "
                    "	dis7[7] -> [in7]col7; "
                    "	dis7[8] -> [in7]col8; "
                    "	dis7[9] -> [in7]col9; "
                    "	dis7[10] -> dis8; "
                    "	dis8[0] -> [in8]col0; "
                    "	dis8[1] -> [in8]col1; "
                    "	dis8[2] -> [in8]col2; "
                    "	dis8[3] -> [in8]col3; "
                    "	dis8[4] -> [in8]col4; "
                    "	dis8[5] -> [in8]col5; "
                    "	dis8[6] -> [in8]col6; "
                    "	dis8[7] -> [in8]col7; "
                    "	dis8[8] -> [in8]col8; "
                    "	dis8[9] -> [in8]col9; "
                    "	dis8[10] -> last_dis; "
                    "	last_dis[0] -> [in9]col0; "
                    "	last_dis[1] -> [in9]col1; "
                    "	last_dis[2] -> [in9]col2; "
                    "	last_dis[3] -> [in9]col3; "
                    "	last_dis[4] -> [in9]col4; "
                    "	last_dis[5] -> [in9]col5; "
                    "	last_dis[6] -> [in9]col6; "
                    "	last_dis[7] -> [in9]col7; "
                    "	last_dis[8] -> [in9]col8; "
                    "	last_dis[9] -> [in9]col9; "
                    "	col0 -> [in0]sink; "
                    "	col1 -> [in1]sink; "
                    "	col2 -> [in2]sink; "
                    "	col3 -> [in3]sink; "
                    "	col4 -> [in4]sink; "
                    "	col5 -> [in5]sink; "
                    "	col6 -> [in6]sink; "
                    "	col7 -> [in7]sink; "
                    "	col8 -> [in8]sink; "
                    "	col9 -> [in9]sink; "
                    "   sink -> output; "
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
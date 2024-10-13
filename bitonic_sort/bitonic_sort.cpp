#include <algorithm>
#include <random>
#include <vector>
#include <numeric>
#include <cmath>

#include "mpi.h"
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

enum sort_type {
    sorted,
    ran,
    one_percent,
    reverse
};

#define MASTER 0
#define UP 0
#define DOWN 1

auto data_init_local = "data_init_local";
auto comm = "comm";
auto comm_small = "comm_small";
auto comm_large = "comm_large";
auto comp = "comp";
auto comp_small = "comp_small";
auto comp_large = "comp_large";
auto correctness_check = "correctness_check";

void data_init(const int taskid, const int numtasks, const int n_each, std::vector<int>& data, sort_type sort_type) {
    data.resize(n_each);
    if (sort_type == sorted) {
        std::iota(data.begin(), data.end(), taskid * n_each);
    } else if (sort_type == reverse) {
        std::iota(data.begin(), data.end(), (numtasks - taskid) * n_each);
        std::reverse(data.begin(), data.end());
    } else if (sort_type == ran) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(INT32_MIN, INT32_MAX);

        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    } else if (sort_type == one_percent) {
        std::iota(data.begin(), data.end(), taskid * n_each);

        int perturb_count = static_cast<int>(n_each * 0.01);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> index_dist(0, n_each - 1);
        std::uniform_int_distribution<> value_dist(INT32_MIN, INT32_MAX);

        for (int i = 0; i < perturb_count; ++i) {
            int idx = index_dist(gen);
            data[idx] = value_dist(gen);
        }

        std::uniform_int_distribution<> task_dist(0, numtasks - 1);
        int partner_task = task_dist(gen);

        while (partner_task == taskid) {
            partner_task = task_dist(gen);
        }

        std::vector<int> recv_data(perturb_count);

        CALI_MARK_BEGIN(comm);
        CALI_MARK_BEGIN(comm_small);
        MPI_Sendrecv(&data[0], perturb_count, MPI_INT, partner_task, 0,
                     &recv_data[0], perturb_count, MPI_INT, partner_task, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        CALI_MARK_END(comm_small);
        CALI_MARK_END(comm);

        for (int i = 0; i < perturb_count; ++i) {
            data[i] = recv_data[i];
        }
    }
}

int main(int argc, char* argv[]) {
    CALI_CXX_MARK_FUNCTION;

    int n;
    std::string sort_type_str;
    if (argc != 3) {
        std::cout << ("Usage: bitonic_sort n sort_type\n");
        return 1;
    } else {
        n = atoi(argv[1]);
        sort_type_str = argv[2];
    }

    sort_type sort_type = sorted;
    switch (sort_type_str) {
        case "Sorted":
            sort_type = sorted;
            break;
        case "Random":
            sort_type = ran;
            break;
        case "1_perc_perturbed":
            sort_type = one_percent;
            break;
        case "ReverseSorted":
            sort_type = reverse;
            break;
        default:
            sort_type = sorted;
    }

    int numtasks, /* number of tasks in partition */
            taskid, /* a task identifier */
            numworkers, /* number of worker tasks */
            source, /* task id of message source */
            dest, /* task id of message destination */
            mtype; /* message type */
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    int n_each = n / numtasks;

    adiak::init(nullptr);
    adiak::launchdate();
    adiak::libraries();
    adiak::cmdline();
    adiak::clustername();
    adiak::value("algorithm", "bitonic");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", n);
    adiak::value("input_type", sort_type_str);
    adiak::value("num_procs", numtasks);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 47);
    adiak::value("implementation_source", "handwritten");

    auto data = std::vector<int>();

    // Initialize data across all tasks
    CALI_MARK_BEGIN(data_init_local);
    data_init(taskid, numtasks, n_each, data, sort_type);
    CALI_MARK_END(data_init_local);

    // Sort data locally and calculate log_numtasks
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    std::sort(data.begin(), data.end());
    CALI_MARK_END(comp_large);

    CALI_MARK_BEGIN(comp_small);
    const auto log_numtasks = static_cast<int>(log2(numtasks));
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);

    for (int s = 0; s < log_numtasks; ++s) {
        for (int t = s; t >= 0; --t) {
            // Determine partner
        }
    }

    return 0;
}


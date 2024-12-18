#include <algorithm>
#include <random>
#include <vector>
#include <numeric>
#include <cmath>
#include <sstream>

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
        std::iota(data.begin(), data.end(), (numtasks - taskid - 1) * n_each);
        std::reverse(data.begin(), data.end());
    } else if (sort_type == ran) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(INT32_MIN, INT32_MAX);

        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    } else if (sort_type == one_percent) {
        // Initialize data as sorted
        std::iota(data.begin(), data.end(), taskid * n_each);

        int perturb_count = static_cast<int>(n_each * 0.01);
        if (perturb_count == 0) perturb_count = 1;

        // Randomly perturb 1% of the data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> index_dist(0, n_each - 1);
        std::uniform_int_distribution<> value_dist(INT32_MIN, INT32_MAX);

        for (int i = 0; i < perturb_count; ++i) {
            int idx = index_dist(gen);
            data[idx] = value_dist(gen);
        }

        int partner_task;
        if (numtasks % 2 == 0) {
            // For even number of tasks, pair taskid with taskid ^ 1
            partner_task = taskid ^ 1;
        } else {
            // For odd number of tasks, pair taskid with (taskid + 1) % numtasks
            partner_task = (taskid + 1) % numtasks;
        }

        // Ensure partner_task is within bounds
        if (partner_task >= numtasks) {
            partner_task = taskid;
        }

        if (partner_task != taskid) {
            std::vector<int> recv_data(perturb_count);

            // Exchange data with partner
            MPI_Sendrecv(&data[0], perturb_count, MPI_INT, partner_task, 0,
                         &recv_data[0], perturb_count, MPI_INT, partner_task, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Replace first perturb_count elements with received data
            for (int i = 0; i < perturb_count; ++i) {
                data[i] = recv_data[i];
            }
        }
    }
}

int main(int argc, char* argv[]) {
    CALI_CXX_MARK_FUNCTION;

    if (argc != 3) {
        std::cout << ("Usage: bitonic_sort n sort_type\n");
        return 1;
    }
    const auto n = atoi(argv[1]);
    const auto sort_type_str = argv[2];

    sort_type sort_type = sorted;
    if (sort_type_str == "Sorted") {
        sort_type = sorted;
    } else if (sort_type_str == "Random") {
        sort_type = ran;
    } else if (sort_type_str == "1_perc_perturbed") {
        sort_type = one_percent;
    } else if (sort_type_str == "ReverseSorted") {
        sort_type = reverse;
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
    adiak::value("group_num", 24);
    adiak::value("implementation_source", "online");

    auto local_data = std::vector<int>();

    // Initialize data across all tasks
    CALI_MARK_BEGIN(data_init_local);
    data_init(taskid, numtasks, n_each, local_data, sort_type);
    CALI_MARK_END(data_init_local);

    // Sort data locally
    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    std::sort(local_data.begin(), local_data.end());
    CALI_MARK_END(comp_large);
    CALI_MARK_END(comp);

    for (int s = 2; s <= numtasks; s <<= 1) {
        for (int t = s >> 1; t > 0; t >>= 1) {
            CALI_MARK_BEGIN(comp);
            CALI_MARK_BEGIN(comp_small);
            // Determine partner
            auto partner_task = taskid ^ t;
            CALI_MARK_END(comp_small);
            CALI_MARK_END(comp);

            if (partner_task < numtasks) {
                CALI_MARK_BEGIN(comp);
                CALI_MARK_BEGIN(comp_small);
                // Determine direction based on bit at s in taskid
                const auto direction = ((taskid & s) == 0) ? UP : DOWN;

                // Data exchange with partner
                auto partner_data = std::vector<int>(n_each);
                CALI_MARK_END(comp_small);
                CALI_MARK_END(comp);

                CALI_MARK_BEGIN(comm);
                CALI_MARK_BEGIN(comm_large);
                if (taskid < partner_task) {
                    MPI_Send(local_data.data(), n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD);
                    MPI_Recv(partner_data.data(), n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD, &status);
                } else {
                    MPI_Recv(partner_data.data(), n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD, &status);
                    MPI_Send(local_data.data(), n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD);
                }
                CALI_MARK_END(comm_large);
                CALI_MARK_END(comm);

                CALI_MARK_BEGIN(comp);
                CALI_MARK_BEGIN(comp_large);
                // Combine data with partner
                auto merged_data = std::vector<int>(n_each * 2);
                std::merge(local_data.begin(), local_data.end(), partner_data.begin(), partner_data.end(), merged_data.begin());

                // Split data with partner
                if ((direction == UP && taskid < partner_task) || (direction == DOWN && taskid > partner_task)) {
                    std::copy(merged_data.begin(), merged_data.begin() + n_each, local_data.begin());
                } else {
                    std::copy(merged_data.begin() + n_each, merged_data.end(), local_data.begin());
                }
                CALI_MARK_END(comp_large);
                CALI_MARK_END(comp);
            }
            CALI_MARK_BEGIN(comm);
            MPI_Barrier(MPI_COMM_WORLD);
            CALI_MARK_END(comm);
        }
    }

    // // Print local data (debugging)
    // std::stringstream ss;
    // bool first = true;
    // for (int i : local_data) {
    //     if (!first)
    //         ss << ", ";
    //     ss << i;
    //     first = false;
    // }
    //
    // std::cout << "Task " << taskid << " Data: " << ss.str() << std::endl;

    // Correctness check
    CALI_MARK_BEGIN(correctness_check);
    // Ensure data is sorted locally
    for (size_t i = 1; i < local_data.size(); ++i) {
        if (local_data[i - 1] > local_data[i]) {
            std::cout << "Task " << taskid << " local data is not sorted!" << std::endl;
            MPI_Finalize();
            return 1;
        }
    }

    // Ensure order across tasks
    if (taskid < numtasks - 1) {
        // Send our last element to the next task
        MPI_Send(&local_data[n_each - 1], 1, MPI_INT, taskid + 1, 0, MPI_COMM_WORLD);
    }

    if (taskid > 0) {
        int neighbor_last;
        // Receive the last element from the previous task
        MPI_Recv(&neighbor_last, 1, MPI_INT, taskid - 1, 0, MPI_COMM_WORLD, &status);
        if (neighbor_last > local_data[0]) {
            std::cout << "Task " << taskid << " data is not in global order!" << std::endl;
            MPI_Finalize();
            return 1;
        }
    }
    CALI_MARK_END(correctness_check);

    MPI_Finalize();
    return 0;
}

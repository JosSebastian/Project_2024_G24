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
        if (perturb_count == 0) perturb_count = 1; // Ensure at least one element is perturbed

        // Randomly perturb 1% of the data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> index_dist(0, n_each - 1);
        std::uniform_int_distribution<> value_dist(INT32_MIN, INT32_MAX);

        for (int i = 0; i < perturb_count; ++i) {
            int idx = index_dist(gen);
            data[idx] = value_dist(gen);
        }

        // Determine partner task in a symmetric way
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
            partner_task = taskid; // No partner; skip exchange
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

void mergeArrays(std::vector<int>& arr1, std::vector<int>& arr2, int n, int num_procs, int merging_arrays){
    auto res = std::vector<int>();
    int size = arr1.size();
    int merged1 = 0;
    int merged2 = 0;
    while(merged1 < size && merged2 < size){
        if(arr1.at(merged1) <= arr2.at(merged2)){
            res.push_back(arr1.at(merged1));
            merged1++;
        } else {
            res.push_back(arr2.at(merged2));
            merged2++;
        }
    }
    if(merged1 < size){
        while(merged1 < size){
            res.push_back(arr1.at(merged1));
            merged1++;
        }
    }
    if(merged2 < size){
        while(merged2 < size){
            res.push_back(arr2.at(merged2));
            merged2++;
        }
    }

    arr1 = res;
}

int main(int argc, char* argv[]) {
    CALI_CXX_MARK_FUNCTION;

    if (argc != 3) {
        std::cout << ("Usage: merge_sort n sort_type\n");
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
    adiak::value("algorithm", "merge");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", n);
    adiak::value("input_type", sort_type_str);
    adiak::value("num_procs", numtasks);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 24);
    adiak::value("implementation_source", "handwritten");

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

    // Merge arrays
    int merging_arrays = 2;

    while(merging_arrays <= numtasks){
        if(taskid % merging_arrays == 0){
            // receive from (p + (merging_arrays/2))
            int sender_task = taskid + (merging_arrays / 2);
            int len = n_each * merging_arrays / 2;
            auto received_data = std::vector<int>(len);
	    CALI_MARK_BEGIN(comm);
	    CALI_MARK_BEGIN(comm_large);
            MPI_Recv(received_data.data(), len, MPI_INT, sender_task, 0, MPI_COMM_WORLD, &status);
	    CALI_MARK_END(comm_large);
	    CALI_MARK_END(comm);

	    CALI_MARK_BEGIN(comp);
            CALI_MARK_BEGIN(comp_small);
            mergeArrays(local_data, received_data, n, numtasks, merging_arrays);
	    CALI_MARK_END(comp_small);
	    CALI_MARK_END(comp);
        }
        if(taskid % merging_arrays == (merging_arrays/2)){
            // send to (p - (merging_arrays/2))
            int receiver_task = taskid - (merging_arrays / 2);
            int len = n_each * merging_arrays / 2;
	    CALI_MARK_BEGIN(comm);
	    CALI_MARK_BEGIN(comm_large);
            MPI_Send(local_data.data(), len, MPI_INT, receiver_task, 0, MPI_COMM_WORLD);
	    CALI_MARK_END(comm_large);
	    CALI_MARK_END(comm);
        }
        merging_arrays *= 2;
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
    if(taskid == 0){
    	for (size_t i = 1; i < local_data.size(); ++i) {
        	if (local_data[i - 1] > local_data[i]) {
            		std::cout << "Task " << taskid << " local data is not sorted!" << std::endl;
            		MPI_Finalize();
            		return 1;
        	}
    	}
	std::cout << "Local data is sorted! Number of elements = " << local_data.size() << std::endl;
    }

    // Ensure order across tasks
//    if (taskid < numtasks - 1) {
        // Send our last element to the next task
//        MPI_Send(&local_data[n_each - 1], 1, MPI_INT, taskid + 1, 0, MPI_COMM_WORLD);
//    }

//    if (taskid > 0) {
//        int neighbor_last;
        // Receive the last element from the previous task
//        MPI_Recv(&neighbor_last, 1, MPI_INT, taskid - 1, 0, MPI_COMM_WORLD, &status);
//        if (neighbor_last > local_data[0]) {
//            std::cout << "Task " << taskid << " data is not in global order!" << std::endl;
//            MPI_Finalize();
//            return 1;
//        }
//    }
    CALI_MARK_END(correctness_check);

    MPI_Finalize();
    return 0;
}

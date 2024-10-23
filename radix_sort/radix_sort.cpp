#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <climits>
#include <mpi.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

enum sort_type {
    sorted,
    ran,
    one_percent,
    reverse_sort
};

#define MASTER 0

void data_init(const int taskid, const int numtasks, const int n_each, std::vector<int>& data, sort_type sort_type);
void counting_sort(std::vector<int>& data, int exp);
void radix_sort(std::vector<int>& data, int max_digit);

int main(int argc, char* argv[]) {
    CALI_CXX_MARK_FUNCTION;

    if (argc != 3) {
        std::cout << "Usage: radix_sort n sort_type\n";
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
        sort_type = reverse_sort;
    }

    int numtasks, taskid;
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
    adiak::value("algorithm", "radix_sort");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", n);
    adiak::value("input_type", sort_type_str);
    adiak::value("num_procs", numtasks);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 24);
    adiak::value("implementation_source", "handwritten");

    std::vector<int> array;
    if (taskid == MASTER) {
        array.resize(n);
        CALI_MARK_BEGIN("computation_data_init");
        data_init(taskid, numtasks, n, array, sort_type);
        CALI_MARK_END("computation_data_init");
    }

    std::vector<int> local_array(n_each);

    // distribute data to workers
    CALI_MARK_BEGIN("communication_data_distribution");
    MPI_Scatter(array.data(), n_each, MPI_INT, local_array.data(), n_each, MPI_INT, MASTER, MPI_COMM_WORLD);
    CALI_MARK_END("communication_data_distribution");

    // local radix sort
    int max_val = *std::max_element(local_array.begin(), local_array.end());
    int max_digit = 0;
    while (max_val > 0) {
        max_digit++;
        max_val /= 10;
    }

    CALI_MARK_BEGIN("computation_local_sort");
    radix_sort(local_array, max_digit);
    CALI_MARK_END("computation_local_sort");

    // gather sorted data at the master process
    std::vector<int> sorted_array;
    if (taskid == MASTER) {
        sorted_array.resize(n);
    }

    CALI_MARK_BEGIN("communication_data_gathering");
    MPI_Gather(local_array.data(), n_each, MPI_INT, sorted_array.data(), n_each, MPI_INT, MASTER, MPI_COMM_WORLD);
    CALI_MARK_END("communication_data_gathering");

    // master merges sorted data
    if (taskid == MASTER) {
        CALI_MARK_BEGIN("computation_final_merge");
        std::inplace_merge(sorted_array.begin(), sorted_array.begin() + n_each, sorted_array.end());
        CALI_MARK_END("computation_final_merge");

        // correctness 
        CALI_MARK_BEGIN("computation_correctness_check");
        for (size_t i = 1; i < sorted_array.size(); ++i) {
            if (sorted_array[i - 1] > sorted_array[i]) {
                std::cout << "Global data is not sorted!" << std::endl;
                MPI_Finalize();
                return 1;
            }
        }
        CALI_MARK_END("computation_correctness_check");
    }

    MPI_Finalize();
    return 0;
}

void counting_sort(std::vector<int>& data, int exp) {
    int n = data.size();
    std::vector<int> output(n);
    int count[10] = {0};

    for (int i = 0; i < n; i++) {
        int digit = (data[i] / exp) % 10;
        count[digit]++;
    }

    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    for (int i = n - 1; i >= 0; i--) {
        int digit = (data[i] / exp) % 10;
        output[count[digit] - 1] = data[i];
        count[digit]--;
    }

    for (int i = 0; i < n; i++) {
        data[i] = output[i];
    }
}

void radix_sort(std::vector<int>& data, int max_digit) {
    for (int exp = 1; max_digit > 0; exp *= 10, max_digit--) {
        counting_sort(data, exp);
    }
}

void data_init(const int taskid, const int numtasks, const int n_each, std::vector<int>& data, sort_type sort_type) {
    data.resize(n_each);
    if (sort_type == sorted) {
        std::iota(data.begin(), data.end(), taskid * n_each);
    } else if (sort_type == reverse_sort) {
        std::iota(data.begin(), data.end(), (numtasks - taskid - 1) * n_each);
        std::reverse(data.begin(), data.end());
    } else if (sort_type == ran) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(INT32_MIN, INT32_MAX);

        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    } else if (sort_type == one_percent) {
        std::iota(data.begin(), data.end(), taskid * n_each);

        int perturb_count = static_cast<int>(n_each * 0.01);
        if (perturb_count == 0) perturb_count = 1;

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
            partner_task = taskid ^ 1;
        } else {
            partner_task = (taskid + 1) % numtasks;
        }

        if (partner_task >= numtasks) {
            partner_task = taskid;
        }

        if (partner_task != taskid) {
            std::vector<int> recv_data(perturb_count);

            CALI_MARK_BEGIN("communication_sendrecv");
            MPI_Sendrecv(&data[0], perturb_count, MPI_INT, partner_task, 0,
                         &recv_data[0], perturb_count, MPI_INT, partner_task, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            CALI_MARK_END("communication_sendrecv");

            CALI_MARK_BEGIN("computation_update_data");
            for (int i = 0; i < perturb_count; ++i) {
                data[i] = recv_data[i];
            }
            CALI_MARK_END("computation_update_data");
        }
    }
}
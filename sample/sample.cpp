
#include <cstdlib>
#include <cmath>
#include <climits>

#include <iostream>
using std::cout, std::endl;

#include <random>
using std::random_device, std::mt19937, std::uniform_int_distribution;
#include <algorithm>
using std::sort, std::is_sorted;
using std::swap, std::shuffle, std::reverse;

#include <vector>
using std::vector;
#include <string>
using std::string;

#include <mpi.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>

#include <adiak.hpp>

#define MASTER 0

void Start(int process, int processes, int size, int type, int &subsize, int &subtype, vector<int> &subarray, bool &sorted)
{
    subsize = size / processes;
    subtype = type;
    subarray.resize(subsize);

    if (type == 0)
    {
        for (int index = 0; index < subsize; index++)
        {
            subarray.at(index) = (process * subsize) + index;
        }
    }

    if (type == 1)
    {
        for (int index = 0; index < subsize; index++)
        {
            subarray.at(index) = (process * subsize) + index;
        }

        if (process == 0)
        {
            int perturb = 0.005 * size < subsize ? int(0.005 * size) : subsize;
            for (int index = 0; index < perturb; index++)
            {
                subarray.at(index) = (processes * subsize) - ((process * subsize) + index + 1);
            }
        }
        if (process == processes - 1)
        {
            int perturb = 0.005 * size < subsize ? int(0.005 * size) : subsize;
            for (int index = 0; index < perturb; index++)
            {
                subarray.at(subarray.size() - index - 1) = index;
            }
        }
    }
    else if (type == 2)
    {
        random_device device;
        mt19937 generator(device());
        uniform_int_distribution<int> distribution(INT_MIN, INT_MAX);

        for (int index = 0; index < subsize; index++)
        {
            subarray.at(index) = distribution(generator);
        }
    }
    if (type == 3)
    {
        for (int index = 0; index < subsize; index++)
        {
            subarray.at(index) = (processes * subsize) - ((process * subsize) + index + 1);
        }
    }
}

void End(int process, int processes, int size, int type, int &subsize, int &subtype, std::vector<int> &subarray, bool &sorted)
{
    bool sort = is_sorted(subarray.begin(), subarray.end());

    int bucket = subarray.size();
    vector<int> buckets(processes);
    MPI_Gather(&bucket, 1, MPI_INT, buckets.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(buckets.data(), processes, MPI_INT, 0, MPI_COMM_WORLD);

    int left = -1;
    for (int index = process - 1; index >= 0; index--)
    {
        if (buckets.at(index) != 0)
        {
            left = index;
            break;
        }
    }

    int right = -1;
    for (int index = process + 1; index < processes; index++)
    {
        if (buckets.at(index) != 0)
        {
            right = index;
            break;
        }
    }

    if (!(buckets.at(process) == 0 || (left == -1 && right == -1)))
    {
        int l, r;
        if (left == -1)
        {
            l = subarray.at(subarray.size() - 1);
            MPI_Recv(&r, 1, MPI_INT, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (l > r)
            {
                sort = false;
            }
        }
        else if (right == -1)
        {
            r = subarray.at(0);
            MPI_Send(&r, 1, MPI_INT, left, 0, MPI_COMM_WORLD);
        }
        else
        {
            r = subarray.at(0);
            MPI_Send(&r, 1, MPI_INT, left, 0, MPI_COMM_WORLD);
            l = subarray.at(subarray.size() - 1);
            MPI_Recv(&r, 1, MPI_INT, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (l > r)
            {
                sort = false;
            }
        }
    }

    bool status = false;
    MPI_Reduce(&sort, &status, 1, MPI_C_BOOL, MPI_LAND, MASTER, MPI_COMM_WORLD);

    sorted = status;
}

int main(int argc, char const *argv[])
{
    CALI_CXX_MARK_FUNCTION;

    int size, type;
    bool sorted;
    {
        if (argc != 3)
        {
            cout << "Usage: ./sample.cpp <size> <type>" << endl;
            return 1;
        }

        int s = atoi(argv[1]);
        if (s <= 0)
        {
            cout << "ERROR: Array Size" << endl;
            return 1;
        }
        size = static_cast<int>(pow(2, s));

        int t = atoi(argv[2]);
        if (t < 0 || t > 3)
        {
            cout << "ERROR: Array Type" << endl;
            return 1;
        }
        type = static_cast<int>(t);
    }

    double total_time;

    const char *data_init_runtime = "data_init_runtime";
    const char *comm = "comm";
    const char *comm_small = "comm_small";
    const char *comm_large = "comm_large";
    const char *comp = "comp";
    const char *comp_small = "comp_small";
    const char *comp_large = "comp_large";
    const char *correctness_check = "correctness_check";

    int processes, process;
    MPI_Init(&argc, (char ***)&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process);
    MPI_Status status;

    cali::ConfigManager manager;
    manager.start();

    if (process == MASTER)
        cout << "Sample Sort: "
             << "Size" << "(" << size << ")"
             << " - "
             << "Type" << "(" << type << ")"
             << endl;

    int subsize, subtype;
    vector<int> subarray;

    CALI_MARK_BEGIN(data_init_runtime);
    Start(process, processes, size, type, subsize, subtype, subarray, sorted);
    CALI_MARK_END(data_init_runtime);

    double total_time_start = MPI_Wtime();

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    sort(subarray.begin(), subarray.end());
    CALI_MARK_END(comp_large);

    CALI_MARK_BEGIN(comp_small);
    vector<int> candidates(processes - 1);
    for (int index = 0; index < processes - 1; index++)
    {
        candidates.at(index) = subarray.at(index * (subsize / (processes - 1)));
    }
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    vector<int> samples(processes * (processes - 1));
    MPI_Gather(candidates.data(), processes - 1, MPI_INT, samples.data(), processes - 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    sort(samples.begin(), samples.end());
    CALI_MARK_END(comp_small);

    CALI_MARK_BEGIN(comp_small);
    vector<int> splitters(processes - 1);
    for (int index = 0; index < processes - 1; index++)
    {
        splitters.at(index) = samples.at((index + 1) * (processes - 1));
    }
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_small);
    MPI_Bcast(splitters.data(), processes - 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    CALI_MARK_END(comm_small);
    CALI_MARK_END(comm);

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_large);
    vector<vector<int>> buckets(processes);
    for (int index = 0; index < subsize; index++)
    {
        int bucket = 0;
        while (bucket < processes - 1 && subarray.at(index) >= splitters.at(bucket))
        {
            bucket++;
        }

        if (bucket < processes - 1)
        {
            buckets.at(bucket).push_back(subarray.at(index));
        }
        else
        {
            buckets.at(processes - 1).push_back(subarray.at(index));
        }
    }
    CALI_MARK_END(comp_large);
    CALI_MARK_END(comp);

    subarray = buckets.at(process);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    for (int index = 0; index < processes; index++)
    {
        if (process != index)
        {
            int bucket = static_cast<int>(buckets.at(index).size());
            MPI_Send(&bucket, 1, MPI_INT, index, 0, MPI_COMM_WORLD);
            MPI_Send(buckets.at(index).data(), bucket, MPI_INT, index, 0, MPI_COMM_WORLD);
        }
    }

    for (int index = 0; index < processes; index++)
    {
        if (process != index)
        {
            int bucket;
            MPI_Recv(&bucket, 1, MPI_INT, index, 0, MPI_COMM_WORLD, &status);

            vector<int> temporary(bucket);
            MPI_Recv(temporary.data(), bucket, MPI_INT, index, 0, MPI_COMM_WORLD, &status);

            CALI_MARK_BEGIN(comp);
            CALI_MARK_BEGIN(comp_large);
            vector<int> result(subarray.size() + temporary.size());
            merge(subarray.begin(), subarray.end(), temporary.begin(), temporary.end(), result.begin());
            subarray = result;
            CALI_MARK_END(comp_large);
            CALI_MARK_END(comp);
        }
    }
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    double total_time_stop = MPI_Wtime();

    CALI_MARK_BEGIN(correctness_check);
    End(process, processes, size, type, subsize, subtype, subarray, sorted);
    CALI_MARK_END(correctness_check);

    if (process == MASTER)
        cout << "Sample Sort: "
             << (sorted ? "Success" : "Failure")
             << endl;

    adiak::init(NULL);
    adiak::launchdate();
    adiak::libraries();
    adiak::cmdline();
    adiak::clustername();
    adiak::value("algorithm", "sample");
    adiak::value("programming_model", "mpi");
    adiak::value("data_type", "int");
    adiak::value("size_of_data_type", sizeof(int));
    adiak::value("input_size", size);
    adiak::value("num_procs", processes);
    adiak::value("scalability", "strong");
    adiak::value("group_num", 24);
    adiak::value("implementation_source", "handwritten");

    if (type == 0)
        adiak::value("input_type", "Sorted");
    else if (type == 1)
        adiak::value("input_type", "1_perc_perturbed");
    else if (type == 2)
        adiak::value("input_type", "Random");
    else if (type == 3)
        adiak::value("input_type", "ReverseSorted");

    total_time = total_time_stop - total_time_start;
    adiak::value("whole_computation", total_time);

    if (process == MASTER)
        cout << "Sample Sort: "
             << total_time << " seconds"
             << endl;

    manager.stop();
    manager.flush();

    MPI_Finalize();
}

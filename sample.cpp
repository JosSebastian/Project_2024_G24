
#include <cstdlib>
#include <cmath>

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

#define MASTER 0

void Start(int process, int processes, int size, int type, int &subsize, int &subtype, vector<int> &subarray)
{
    if (process == MASTER)
        cout << "Sample Sort: "
             << "Size" << "(" << size << ")"
             << " - "
             << "Type" << "(" << type << ")"
             << endl;

    subsize = size / processes;
    subtype = type;
    subarray.resize(subsize);

    vector<int> array(size);
    for (int index = 0; index < size; index++)
    {
        array.at(index) = index;
    }

    if (type == 1)
    {
        random_device device;
        mt19937 generator(device());
        uniform_int_distribution<int> distribution(0, size - 1);

        int perturb = size / 100;
        for (int i = 0; i < perturb; ++i)
        {
            int index1 = distribution(generator);
            int index2 = distribution(generator);
            swap(array.at(index1), array.at(index2));
        }
    }
    else if (type == 2)
    {
        random_device device;
        mt19937 generator(device());
        shuffle(array.begin(), array.end(), generator);
    }
    if (type == 3)
    {
        reverse(array.begin(), array.end());
    }

    MPI_Scatter(array.data(), subsize, MPI_INT, subarray.data(), subsize, MPI_INT, MASTER, MPI_COMM_WORLD);
}

void End(int process, int processes, int size, int type, int &subsize, int &subtype, std::vector<int> &subarray)
{
    bool sort = is_sorted(subarray.begin(), subarray.end());

    if (sort)
    {
        MPI_Status status;
        int first, last;
        int before, after;

        if (process == 0)
        {
            last = subarray.at(subarray.size() - 1);

            MPI_Send(&last, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&after, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

            if (last > after)
                sort = false;
        }
        else if (process == processes - 1)
        {
            first = subarray.at(0);

            MPI_Send(&first, 1, MPI_INT, processes - 2, 0, MPI_COMM_WORLD);
            MPI_Recv(&before, 1, MPI_INT, processes - 2, 0, MPI_COMM_WORLD, &status);

            if (first < before)
                sort = false;
        }
        else
        {
            first = subarray.at(0);
            last = subarray.at(subarray.size() - 1);

            MPI_Send(&first, 1, MPI_INT, process - 1, 0, MPI_COMM_WORLD);
            MPI_Send(&last, 1, MPI_INT, process + 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&before, 1, MPI_INT, process - 1, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&after, 1, MPI_INT, process + 1, 0, MPI_COMM_WORLD, &status);

            if (first < before || last > after)
                sort = false;
        }
    }

    bool status = false;
    MPI_Reduce(&sort, &status, 1, MPI_C_BOOL, MPI_LAND, MASTER, MPI_COMM_WORLD);

    if (process == MASTER)
        cout << "Sample Sort: "
             << (status ? "Success" : "Failure")
             << endl;
}

int main(int argc, char const *argv[])
{
    int size, type;
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

    int processes, process;
    MPI_Init(&argc, (char ***)&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process);
    MPI_Status status;

    int subsize, subtype;
    vector<int> subarray;

    Start(process, processes, size, type, subsize, subtype, subarray);

    sort(subarray.begin(), subarray.end());

    vector<int> candidates(processes - 1);
    for (int index = 0; index < processes - 1; index++)
    {
        candidates.at(index) = subarray.at(index * (subsize / (processes - 1)));
    }

    vector<int> samples(processes * (processes - 1));
    MPI_Gather(candidates.data(), processes - 1, MPI_INT, samples.data(), processes - 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    sort(samples.begin(), samples.end());

    vector<int> splitters(processes - 1);
    for (int index = 0; index < processes - 1; index++)
    {
        splitters.at(index) = samples.at((index + 1) * (processes - 1));
    }
    MPI_Bcast(splitters.data(), processes - 1, MPI_INT, MASTER, MPI_COMM_WORLD);

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

    subarray = buckets.at(process);

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

            vector<int> result(subarray.size() + temporary.size());
            merge(subarray.begin(), subarray.end(), temporary.begin(), temporary.end(), result.begin());
            subarray = result;
        }
    }

    End(process, processes, size, type, subsize, subtype, subarray);

    MPI_Finalize();
}

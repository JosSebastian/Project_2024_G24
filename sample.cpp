
#include <cstdlib>
#include <cmath>

#include <iostream>
using std::cout, std::endl;

#include <random>
using std::random_device, std::mt19937, std::uniform_int_distribution;
#include <algorithm>
using std::swap, std::shuffle, std::reverse, std::merge, std::copy;

#include <vector>
using std::vector;

#include <mpi.h>

#define MASTER 0

void Configure(int size, int type, vector<int> &array)
{
    array.resize(size);
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
        for (int i = 0; i < perturb / 2; ++i)
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
    if (type == 2)
    {
        reverse(array.begin(), array.end());
    }
}

void Check(int size, int type, vector<int> &array)
{
    bool sort = true;
    for (int index = 0; index < size - 1; ++index)
    {
        if (array.at(index) > array.at(index + 1))
        {
            sort = false;
            break;
        }
    }

    cout << "Sample Sort: " << (sort ? "Success" : "Failure") << endl;
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: ./sample.cpp <size> <type>" << endl;
        return 1;
    }

    int size = static_cast<int>(pow(2, atoi(argv[1])));

    int type = atoi(argv[2]);
    char *types[] = {"sort", "perturb", "random", "reverse"};

    if (size <= 0)
    {
        cout << "ERROR: Array Size" << endl;
        return 1;
    }
    if (type < 0 || type > 3)
    {
        cout << "ERROR: Array Type" << endl;
        return 1;
    }

    int processes, process;
    MPI_Init(&argc, (char ***)&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process);

    if (process == MASTER)
    {
        cout << "Sample Sort: " << "Size(" << size << ") - Type(" << types[type] << ")" << endl;
    }

    vector<int> array;
    if (process == MASTER)
    {
        Configure(size, type, array);
    }

    int subsize = size / processes;
    vector<int> subarray(subsize);

    MPI_Scatter(array.data(), subsize, MPI_INT, subarray.data(), subsize, MPI_INT, MASTER, MPI_COMM_WORLD);

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

    int partner;
    bool complete = false;
    int bitmask = 1;
    MPI_Status status;

    while (!complete && bitmask < processes)
    {
        partner = process ^ bitmask;
        if (process < partner)
        {
            for (int index = 0; index < processes; index++)
            {
                int bucket;
                MPI_Recv(&bucket, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, &status);

                vector<int> temporary(bucket);
                MPI_Recv(temporary.data(), bucket, MPI_INT, partner, 0, MPI_COMM_WORLD, &status);

                vector<int> result(buckets.at(index).size() + temporary.size());
                merge(buckets.at(index).begin(), buckets.at(index).end(), temporary.begin(), temporary.end(), result.begin());
                buckets.at(index) = result;
            }
            bitmask <<= 1;
        }
        else
        {
            for (int index = 0; index < processes; index++)
            {
                int bucket = static_cast<int>(buckets.at(index).size());
                MPI_Send(&bucket, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
                MPI_Send(buckets.at(index).data(), bucket, MPI_INT, partner, 0, MPI_COMM_WORLD);
            }
            complete = true;
        }
    }

    if (process == MASTER)
    {
        int position = 0;
        for (int index = 0; index < processes; index++)
        {
            int bucket = static_cast<int>(buckets.at(index).size());
            copy(buckets.at(index).begin(), buckets.at(index).end(), array.begin() + position);
            position += bucket;
        }
    }

    if (process == MASTER)
    {
        Check(size, type, array);
    }

    MPI_Finalize();
}
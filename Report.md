# CSCE 435 Group project

## 0. Group number: 24

## 1. Group members:
1. Jos Sebastian
2. Jackson Stone
3. Chase Fletcher
4. Ryan Swetonic

## 2. Project topic (e.g., parallel sorting algorithms)
The topic of this project is the implementation, evaluation, and analysis of various parallel sorting algorithms and how they will behave in various situations with differing problems sizes, number of available processors, and levels of sorting completion.

## 3. Communication
The main method of communication for this group project will be through Slack.

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
    Each process is given an equal (or as close to equal as possible) amount of data which is sorted locally. The processes are then paired up in a network log_2(n) times and merged into bitonic sequences, until on the final merge they are merged into a sorted sequence.

- Sample Sort:
    Sample sort sorts datasets by following steps. First, random samples are selected and sorted to create defined buckets. Next, elements from the original dataset are assigned to these buckets based on their values, and each bucket is then sorted independently. Finally, the sorted buckets are merged to produce the fully sorted dataset.

- Merge Sort:
    First, each processor sequentially sorts its data. Then, arrays are merged 2 at a time, keeping them sorted. This continues until all data is merged into one array.

- Radix Sort:
    Radix sort sorts arrays of numbers by comparing them digit-by-digit. It begins
    with the least significant digit, and groups numbers into buckets based on
    their current digit. This process repeats for all digits, until the number
    with the most digits is fully sorted across all processes.

### 2b. Pseudocode for each parallel algorithm

#### Sample Sort
```
function Start(process, processes, size, type, subsize, subtype, subarray, sorted):
    subsize = size / processes
    subtype = type
    initialize subarray with size subsize

    if type == 0:
        for index from 0 to subsize - 1:
            subarray[index] = (process * subsize) + index

    else if type == 1:
        for index from 0 to subsize - 1:
            subarray[index] = (process * subsize) + index
        if process == 0:
            perturb = min(0.005 * size, subsize)
            for index from 0 to perturb - 1:
                subarray[index] = (processes * subsize) - ((process * subsize) + index + 1)
        else if process == processes - 1:
            perturb = min(0.005 * size, subsize)
            for index from 0 to perturb - 1:
                subarray[subsize - index - 1] = index

    else if type == 2:
        for index from 0 to subsize - 1:
            subarray[index] = random number between 0 and size

    else if type == 3:
        for index from 0 to subsize - 1:
            subarray[index] = (processes * subsize) - ((process * subsize) + index + 1)

function End(process, processes, size, type, subsize, subtype, subarray, sorted):
    sort_status = is_sorted(subarray)

    bucket = size of subarray
    buckets = array of size processes
    MPI_Gather(bucket to buckets from all processes)
    MPI_Bcast buckets to all processes

    left = find left neighbor with non-zero bucket
    right = find right neighbor with non-zero bucket

    if bucket is not zero and left and right are not both -1:
        if left == -1:
            l = last element of subarray
            r = receive from right
            sort_status = sort_status AND (l <= r)
        else if right == -1:
            r = first element of subarray
            send r to left
        else:
            send r to left
            l = last element of subarray
            r = receive from right
            sort_status = sort_status AND (l <= r)

    global_status = AND of sort_status from all processes
    sorted = global_status

function main(argc, argv):
    if argc != 3:
        print usage message and exit

    size = 2 raised to power of atoi(argv[1])
    type = atoi(argv[2])

    initialize MPI and get process and total number of processes

    start timing
    if process == MASTER:
        print "Sample Sort: Size" and "Type"

    initialize subsize, subtype, and subarray
    Start(process, processes, size, type, subsize, subtype, subarray, sorted)

    sort subarray

    gather candidates for sampling from all processes
    sort candidates

    find splitters and broadcast to all processes

    initialize buckets
    distribute elements into buckets based on splitters

    subarray = bucket of current process
    exchange buckets between processes and merge

    End(process, processes, size, type, subsize, subtype, subarray, sorted)

    if process == MASTER:
        print result of sort and time taken

    finalize MPI

```

#### Merge Sort
```
Main:
    Perform sequential sort on each processor

    merging_arrays = 2
    p = processor rank

    while merging_arrays <= num_procs:
        if p % merging_arrays == 0:
            receive from (p + (merging_arrays/2))
            mergeArrays
        if p % merging_arrays == merging_arrays/2:
            send to (p - (merging_arrays/2))
        merging_arrays *= 2

mergeArrays:
	while there are elements unadded in both input arrays:
		append smallest element from input arrays to output array
		advance past element added
	Append remaining elements from one array
	Return output array
```

#### Bitonic Sort
```
main() {
  MASTER, UP = 0
  DOWN = 1

  MPI_Status status;

  n = number of elements to be sorted

  MPI_Init()

  MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  if numtasks is not a power of 2:
    return 1

  n_each = n / numtasks

  local_data = empty array of size n_each

  if taskid == 0:
    A = array of elements to be sorted

  MPI_Scatter(a, n_each, MPI_INT, local_data, n_each, MPI_INT, 0, MPI_COMM_WORLD) // scatter an equal amount of the data among all processes

  // Sort this processes data in UP direction
  bitonic_up(local_data, n_each)

  // Parallel sort
  log_numtasks = log_2(numtasks)
  for (s = 0; s < log_numtasks; s++) {
    for (t = s; t >= 0; t--) {
      // Determine partner
      partner_task = taskid ^ (1 << t)

      // Determine direction based on bit at s + 1 in taskid
      direction = UP if ((taskid >> (s + 1)) mod 2 == 0) else DOWN

      // Data exchange with partner
      partner_data = empty array of size n_each
      if (taskid > partner_task) {
        MPI_Send(local_data, n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD, status)
        MPI_Recv(partner_data, n_each, MPI_INT, partner_task, 0 , MPI_COMM_WORLD, status)
      } else {
        MPI_Recv(partner_data, n_each, MPI_INT, partner_task, 0 , MPI_COMM_WORLD, status)
        MPI_Send(local_data, n_each, MPI_INT, partner_task, 0, MPI_COMM_WORLD, status)
      }

      // Combine data with partner
      merged = merge(local_data, partner_data, direction)

      // Split data with partner
      if direction == UP and taskid < partner or direction == DOWN and rank > partner {
        copy(merged, merged + n_each - 1, local_data)
      } else {
        copy(merged + n_each, merged + n_each * 2 - 1, local_data)
      }
    }
  }

  // Collect data
  MPI_Gather(local_data, n_each, MPI_INT, A, n_each, MPI_INT, 0, MPI_COMM_WORLD)

  // Output
  if taskid == 0 {
    output A
  }

  MPI_Finalize()
}
```

#### Radix Sort
```
p = number of processes
array = inputted array to be sorted
n = problem size
max_digit = maximum number of digits in largest number

id = Process ID
master_process = 0
worker_processes = [1, 2, 3, ..., p - 1]

if id == master_process:
    chunk_size = n / p
    for index, process in worker_processes:
        start = index * chunk_size
        end = (index + 1) * chunk_size
        Send(array[start:end], to=process)

else if id is in worker_process:
    local_array = Receive(from=master_process)

    for exp in range(1, max_digit):

        buckets = ten empty arrays inside a big array
        for number in local_array:
            digit = (number // exp) % 10
            buckets[digit].append(number)

        for i in range(10):
            Send(buckets[i], to=all_other_processes)
            Receive(buckets[i], from=all_other_processes)

        local_array = concatenate(buckets)

    Send(local_array, to=master_process)

if id == master_process:
    for process in worker_processes:
        sorted_chunk = Receive(from=process)
        Append sorted_chunk to the final sorted array

```


### 2c. Evaluation plan - what and how will you measure and compare
- Input Sizes, Input Types, Processes

    - Input Sizes: 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28

    - Input Types: Sorted, Sorted with 1% Perturbed, Random, Reverse Sorted

    - Processes: 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)

### 3a. Caliper instrumentation
Please use the caliper build `/scratch/group/csce435-f24/Caliper/caliper/share/cmake/caliper`
(same as lab2 build.sh) to collect caliper files for each experiment you run.

Your Caliper annotations should result in the following calltree
(use `Thicket.tree()` to see the calltree):
```
main
|_ data_init_X      # X = runtime OR io
|_ comm
|    |_ comm_small
|    |_ comm_large
|_ comp
|    |_ comp_small
|    |_ comp_large
|_ correctness_check
```

Required region annotations:
- `main` - top-level main function.
    - `data_init_X` - the function where input data is generated or read in from file. Use *data_init_runtime* if you are generating the data during the program, and *data_init_io* if you are reading the data from a file.
    - `correctness_check` - function for checking the correctness of the algorithm output (e.g., checking if the resulting data is sorted).
    - `comm` - All communication-related functions in your algorithm should be nested under the `comm` region.
      - Inside the `comm` region, you should create regions to indicate how much data you are communicating (i.e., `comm_small` if you are sending or broadcasting a few values, `comm_large` if you are sending all of your local values).
      - Notice that auxillary functions like MPI_init are not under here.
    - `comp` - All computation functions within your algorithm should be nested under the `comp` region.
      - Inside the `comp` region, you should create regions to indicate how much data you are computing on (i.e., `comp_small` if you are sorting a few values like the splitters, `comp_large` if you are sorting values in the array).
      - Notice that auxillary functions like data_init are not under here.
    - `MPI_X` - You will also see MPI regions in the calltree if using the appropriate MPI profiling configuration (see **Builds/**). Examples shown below.

All functions will be called from `main` and most will be grouped under either `comm` or `comp` regions, representing communication and computation, respectively. You should be timing as many significant functions in your code as possible. **Do not** time print statements or other insignificant operations that may skew the performance measurements.

### **Nesting Code Regions Example** - all computation code regions should be nested in the "comp" parent code region as following:
```
CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_small");
sort_pivots(pivot_arr);
CALI_MARK_END("comp_small");
CALI_MARK_END("comp");

# Other non-computation code
...

CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_large");
sort_values(arr);
CALI_MARK_END("comp_large");
CALI_MARK_END("comp");
```

### **Calltree Example**:
```
# MPI Mergesort
4.695 main
├─ 0.001 MPI_Comm_dup
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Init
├─ 0.000 MPI_Initialized
├─ 2.599 comm
│  ├─ 2.572 MPI_Barrier
│  └─ 0.027 comm_large
│     ├─ 0.011 MPI_Gather
│     └─ 0.016 MPI_Scatter
├─ 0.910 comp
│  └─ 0.909 comp_large
├─ 0.201 data_init_runtime
└─ 0.440 correctness_check
```
### Sample Sort Calltree
```
1.054 main
├─ 0.000 MPI_Comm_dup
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Init
├─ 0.000 MPI_Initialized
├─ 0.006 comm
│  ├─ 0.003 comm_large
│  │  ├─ 0.001 MPI_Recv
│  │  ├─ 0.000 MPI_Send
│  │  └─ 0.003 comp
│  │     └─ 0.003 comp_large
│  └─ 0.002 comm_small
│     ├─ 0.001 MPI_Bcast
│     └─ 0.001 MPI_Gather
├─ 0.025 comp
│  ├─ 0.025 comp_large
│  └─ 0.000 comp_small
├─ 0.005 correctness_check
│  ├─ 0.000 MPI_Bcast
│  ├─ 0.004 MPI_Gather
│  ├─ 0.000 MPI_Recv
│  ├─ 0.000 MPI_Reduce
│  └─ 0.000 MPI_Send
└─ 0.005 data_init_runtime
```

### Bitonic Sort Calltree
```
15.162 main
├─ 0.018 MPI_Comm_dup
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Init
├─ 0.000 MPI_Initialized
├─ 0.213 comm
│  ├─ 0.107 MPI_Barrier
│  └─ 0.104 comm_large
│     ├─ 0.066 MPI_Recv
│     └─ 0.035 MPI_Send
├─ 2.386 comp
│  ├─ 2.275 comp_large
│  └─ 0.108 comp_small
├─ 0.031 correctness_check
│  ├─ 0.000 MPI_Recv
│  └─ 0.001 MPI_Send
└─ 0.063 data_init_local
```

### Merge Sort Calltree
```
0.390 main
├─ 0.000 MPI_Init
├─ 0.000 data_init_local
├─ 0.004 comp
│  ├─ 0.003 comp_large
│  └─ 0.003 comp_small
├─ 0.002 comm
│  └─ 0.002 comm_large
│     ├─ 0.004 MPI_Recv
│     └─ 0.000 MPI_Send
├─ 0.000 correctness_check
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Initialized
├─ 0.000 MPI_Finalized
└─ 0.014 MPI_Comm_dup
```

### Radix Sort Calltree
```
18.637 main
├─ 0.249 MPI_Comm_dup
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Init
├─ 0.000 MPI_Initialized
├─ 0.183 comm
│  └─ 0.183 comm_large
│     ├─ 0.021 MPI_Gather
│     └─ 0.161 MPI_Scatter
├─ 0.276 comp
│  └─ 0.275 comp_large
├─ 0.003 correctness_check
└─ 0.001 data_init_runtime
```

### 3b. Collect Metadata

Have the following code in your programs to collect metadata:
```
adiak::init(NULL);
adiak::launchdate();    // launch date of the job
adiak::libraries();     // Libraries used
adiak::cmdline();       // Command line used to launch the job
adiak::clustername();   // Name of the cluster
adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
adiak::value("programming_model", programming_model); // e.g. "mpi"
adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
```
#### Sample
```json
{
    "algorithm": "Sample Sort",
    "programming_model": "mpi",
    "data_type": "int",
    "size_of_data_type": 4,
    "input_size": 65536,
    "input_type": "Random",
    "num_procs": 32,
    "scalability": "strong",
    "group_num": 24,
    "implementation_source": "handwritten"
}
```
#### Bitonic
```json
{
    "algorithm": "bitonic",
    "programming_model": "mpi",
    "data_type": "int",
    "size_of_data_type": "4",
    "input_size": "4194304",
    "num_procs": "16",
    "scalability": "strong",
    "group_num": "24",
    "implementation_source": "online"
}
```
#### Merge sort
```json
{
    "algorithm": "merge",
    "programming_model": "mpi",
    "data_type": "int",
    "size_of_data_type": "4",
    "input_size": "65536",
    "num_procs": "4",
    "scalability": "strong",
    "group_num": "24",
    "implementation_source": "handwritten"
}
```
#### Radix sort
```json
{
    "algorithm": "radix_sort",
    "programming_model": "mpi",
    "data_type": "int",
    "size_of_data_type": 4,
    "input_size": 16777216,
    "input_type": "Random",
    "num_procs": 64,
    "scalability": "strong",
    "group_num": 24,
    "implementation_source": "handwritten"
}
```
They will show up in the `Thicket.metadata` if the caliper file is read into Thicket.

### **See the `Builds/` directory to find the correct Caliper configurations to get the performance metrics.** They will show up in the `Thicket.dataframe` when the Caliper file is read into Thicket.
## 4. Performance evaluation

# Radix Sort
Here are the total time plots for the Radix Sort implementation:
### Radix Sort: Total Time

<table>
  <tr>
    <td><img src="./radix_sort/plots/Radix_Sort_Normalized_Total_Time_Sorted.png" alt="Total Time Sorted" width="300"/></td>
    <td><img src="./radix_sort/plots/Radix_Sort_Normalized_Total_Time_Reverse_Sorted.png" alt="Total Time Reverse Sorted" width="300"/></td>
  </tr>
  <tr>
    <td><img src="./radix_sort/plots/Radix_Sort_Normalized_Total_Time_1__Perturbed.png" alt="Total Time 1% Perturbed" width="300"/></td>
    <td><img src="./radix_sort/plots/Radix_Sort_Normalized_Total_Time_Randomized.png" alt="Total Time Randomized" width="300"/></td>
  </tr>
</table>

As expected, the general trend that is followed is that as the number of processors increases, the total time for execution decreases. The way the input data is sorted seems to have a negligible impact on execution time. A discrepancy is noted in the 1% perturbed execution at 2^9 processors; however, it is most likely an outlier. Another thing worthy of note is that numbers of processors beyond 2^6 yield negligible (and sometimes slightly longer) total times.

Investigating the communication overhead plots for each input type could give more insight into the observed trends;
### Radix Sort: Communication Time

<table>
  <tr>
    <td><img src="./radix_sort/plots/Max_Comm_Time_Sorted.png" alt="Comm Time Sorted" width="300"/></td>
    <td><img src="./radix_sort/plots/Max_Comm_Time_Reverse_Sorted.png" alt="Comm Time Reverse Sorted" width="300"/></td>
  </tr>
  <tr>
    <td><img src="./radix_sort/plots/max_comm_1_perc_perturbed.png" alt="Comm Time 1% Perturbed" width="300"/></td>
    <td><img src="./radix_sort/plots/Max_Comm_Time_Randomized.png" alt="Comm Time Randomized" width="300"/></td>
  </tr>
</table>
It is apparent that, starting at approximately 2^6 processors, there is additional communication overhead introduced that adds extra runtime. Adding any more processes beyond 32 makes the program spend more time communicating between processes. 

Taking a look at the computation time graphs could help give further insight into what is happenning to cause such diminishing returns when the number of processes is increased beyond 32.


### Radix Sort: Computation Time

<table>
  <tr>
    <td><img src="./radix_sort/plots/Max_Comp_Time_Rank_Sorted.png" alt="Comp Time Sorted" width="300"/></td>
    <td><img src="./radix_sort/plots/Max_Comp_Time_Rank_Reverse_Sorted.png" alt="Comp Time Reverse Sorted" width="300"/></td>
  </tr>
  <tr>
    <td><img src="./radix_sort/plots/max_comp_1_perc_perturbed.png" alt="Comp Time 1% Perturbed" width="300"/></td>
    <td><img src="./radix_sort/plots/Max_Comp_Time_Rank_Randomized.png" alt="Comp Time Randomized" width="300"/></td>
  </tr>
</table>

The computation time is not decreased by any significant margin past the point of 32 processors. This is likely due to the fact that Radix Sort sorts by digits; past 32 processes the ratio of digits in the input size to the amount of work assigned to each process is decreased dramatically, meaning there is little use that can be made by further dividing the sorting task between processes. At this point, adding processes serves to only increase communication overhead, and overall cause negligible decreases or even increases in total program execution time.

### Sample Sort

![Sample Sort - Total Time (Sorted)](./sample_sort/plots/Sample_Sort_-_Total_Time_Sorted.png)
![Sample Sort - Total Time (Reverse Sorted)](./sample_sort/plots/Sample_Sort_-_Total_Time_Reverse_Sorted.png)
![Sample Sort - Total Time (1% Perturbed)](./sample_sort/plots/Sample_Sort_-_Total_Time_1_Pertrubed.png)
![Sample Sort - Total Time (Randomized)](./sample_sort/plots/Sample_Sort_-_Total_Time_Randomized.png)

The plots above show how the sample sort algorithm performs as we scale up the number of processors.

From 2 to 32 processors, there's a clear trend: as we add more processors, the execution time drops, indicating that the algorithm is making the most of the additional resources.

However, things change when we scale up to 64 to 512 processors. In this range, execution time actually starts to increase. This suggests that after a certain point, adding more processors doesn’t help—instead, it adds extra costs related to both computation and communication. The later plots highlight a noticeable rise in time spent on these factors.

When we take a closer look at the sample sort implementation, we see that the main computational cost comes from allocating values into local buckets. The biggest communication cost comes from sending these values to the right processes and merging them back into local buckets.

I did run into some trouble when trying to execute the algorithm on 1024 processors. I suspect this was mainly due to certain parts of the implementation requiring a lot of communication. Each processor needs to connect with every other processor, leading to a staggering number of transactions—specifically, 1024*1024 communications across multiple nodes. This overwhelming demand likely caused the program to time out before it could finish.

![Sample Sort - Computation (Sorted)](./sample_sort/plots/Sample_Sort_-_Computation_Sorted.png)
![Sample Sort - Computation (Reverse Sorted)](./sample_sort/plots/Sample_Sort_-_Computation_Reverse_Sorted.png)
![Sample Sort - Computation (1% Perturbed)](./sample_sort/plots/Sample_Sort_-_Computation_1_Pertrubed.png)
![Sample Sort - Computation (Randomized)](./sample_sort/plots/Sample_Sort_-_Computation_Randomized.png)

![Sample Sort - Communication (Sorted)](./sample_sort/plots/Sample_Sort_-_Communication_Sorted.png)
![Sample Sort - Communication (Reverse Sorted)](./sample_sort/plots/Sample_Sort_-_Communication_Reverse_Sorted.png)
![Sample Sort - Communication (1% Perturbed)](./sample_sort/plots/Sample_Sort_-_Communication_1_Pertrubed.png)
![Sample Sort - Communication (Randomized)](./sample_sort/plots/Sample_Sort_-_Communication_Randomized.png)
### Merge Sort

![Max time/rank - 1% perturbed](./merge_sort/plots/Max_time_per_rank_vs._Number_of_Processes_for_Input_Type__1_perc_perturbed.png)
![Max time/rank - Random](./merge_sort/plots/Max_time_per_rank_vs._Number_of_Processes_for_Input_Type__Random.png)
![Max time/rank - Sorted](./merge_sort/plots/Max_time_per_rank_vs._Number_of_Processes_for_Input_Type__Sorted.png)
![Max time/rank - Reverse](./merge_sort/plots/Max_time_per_rank_vs._Number_of_Processes_for_Input_Type__ReverseSorted.png)

These first graphs show the maximum time taken per rank for the entire main function. I chose to plot max time/rank because the merge sort algorithm sends all the data to a single process, so the process that receives all the data takes the longest, and is a good representation of the total time the algorithm takes. These plots show a distinct decrease in the total time taken as the number of processors increases. The largest impact is seen on a large input size. The algorithm appears to take approximately the same amount of time no matter which input type it is. I do not have any data for 1024 processors due to communication errors on Grace when attempting to run it.

![Max computation time/rank - 1% perturbed](./merge_sort/plots/Max_computation_time_per_rank_vs._Number_of_Processes_for_Input_Type__1_perc_perturbed.png)
![Max computation time/rank - Random](./merge_sort/plots/Max_computation_time_per_rank_vs._Number_of_Processes_for_Input_Type__Random.png)
![Max computation time/rank - Sorted](./merge_sort/plots/Max_computation_time_per_rank_vs._Number_of_Processes_for_Input_Type__Sorted.png)
![Max computation time/rank - Reverse](./merge_sort/plots/Max_computation_time_per_rank_vs._Number_of_Processes_for_Input_Type__ReverseSorted.png)

These plots above show the computation time for the algorithm. These all show very cleanly the decrease of time as the number of processors increases. We again see a large impact on the largest input size, and the biggest effect is seen when moving from 2 to 4 processors before the graph seems to flatten. 

![Max communication time/rank - 1% perturbed](./merge_sort/plots/Max_communication_time_per_rank_vs._Number_of_Processes_for_Input_Type__1_perc_perturbed.png)
![Max communication time/rank - Random](./merge_sort/plots/Max_communication_time_per_rank_vs._Number_of_Processes_for_Input_Type__Random.png)
![Max communication time/rank - Sorted](./merge_sort/plots/Max_communication_time_per_rank_vs._Number_of_Processes_for_Input_Type__Sorted.png)
![Max communication time/rank - Reverse](./merge_sort/plots/Max_communication_time_per_rank_vs._Number_of_Processes_for_Input_Type__ReverseSorted.png)

The final plots show the communication time for the algorithm. The most interesting thing to note is the sharp increase at 64 processors. This jump is due to the communication now happening between multiple nodes rather than just on one node. Communicating is much more costly when happening across multiple nodes as is obvious on the graph. We also see a pattern of increasing communication time as the number of processors increases which makes sense due to more messages being passed because there are more processors.

### Bitonic Sort
Unfortunately, the plots are missing data from many of the 1024 processor count runs. These runs were interrupted by a
bug in grace affecting communication over large processor counts.

#### Strong Scaling Performance
![strong_scaling_avg_across_input_types.png](bitonic_sort/analysis/plots/strong_scaling_avg_across_input_types.png)

This graph shows the average time per process as the number of processes increases, while input size remains constant.
The goal of strong scaling is to see whether adding more processes reduces the time per rank efficiently. The cones
around the line represent the variance, or the difference between minimum and maximum time per rank. As each rank is
running in parallel, reducing the time per rank directly correlates to reducing the wall time of the program.

For larger input sizes, we see a massive decrease in time per rank as the number of processes increases, however this
starts to stabilize around 64 processes. Note that the x axis is logarithmic, so we would expect the trend to be linear
if we were scaling perfectly. Because the trend slows at larger input sizes, this indicates diminishing returns on
adding new processes, which we will explore further in the communication overhead section.

For smaller input sizes, we actually see a trend of increasing time per rank as we increase the number of processes.
This further points to overhead in the program, as the communication portion of the program quickly comes to dominate
the relatively fast serial computation times at these smaller input sizes. This indicates that parallelization is not
ideal for these smaller input sizes, as the overhead of communication outweighs the benefits of parallel computation.

![strong_scaling_avg_across_input_types_total_time.png](bitonic_sort/analysis/plots/strong_scaling_avg_across_input_types_total_time.png)

This plot is similar to the last, except it shows total execution time rather than time per rank. This is effectively
the time per rank multiplied by the number of ranks, so it is a measure of how much resources the program takes to run.
Ideally, and if there were no other overhead from parallelization, the total time would remain constant. As we double
the rank, contributing to an increase in total time, we would see a halving of time per rank, resulting in a constant.

This is not what we see in the graph, further pointing to overhead as we further parallelize the program. Not that this
does not mean we should not parallelize at larger input sizes, as it still is reducing the wall time of the program, but
this further shows the diminishing returns that we do get and the increased total cost of running the program.

#### Weak Scaling Performance
![weak_scaling_avg_across_input_types.png](bitonic_sort/analysis/plots/weak_scaling_avg_across_input_types.png)

This graph shows the average time per rank as we increase both number of processes and input size by the same factor.
In weak scaling, the goal is to keep the amount of work the same in each process as input size increases by also
increasing resources, and thus we would ideally see a flat line.

For the smaller input sizes and process counts, we see a steady increase in average time per rank, pointing to
parallelization overhead. Further increases in process size and input size interestingly show better weak scaling.
This is likely due to the fact that the overhead of parallelization is more outweighed (but not completely, as we still
see a positive trend) by the increased resources and work that the program is doing.

![weak_scaling_avg_across_input_types_total_time.png](bitonic_sort/analysis/plots/weak_scaling_avg_across_input_types_total_time.png)

This plot displays the total execution time of the program as we increase both input size and process count. Ideally, we
would also expect this graph to remain constant, as the total work done by the program is increasing at the same rate as
the resources available to the program. This is not what we see, as the total time increases as we increase input size
and process count. This further points to overhead in the program, as the total time is increasing faster than the extra
resources are able to compensate for the larger input size.

#### Input Type Performance
![input_type_impact_on_performance.png](bitonic_sort/analysis/plots/input_type_impact_on_performance.png)

Before analyzing the communication overhead, and thus overhead of parallelization, we will look at how the initial input
sorting affects program performance. This graph shows the average time per rank for each input type, averaged across
all input sizes and processor counts. While there is some variation, there is also a large variance, and each average is
well withing the variance of the others. This indicates that the initial sorting of the input data does not have effect
on the overall performance of the program, which makes sense as bitonic sort is not a stable sorting algorithm.

#### Communication Overhead
![communication_overhead_facetgrid.png](bitonic_sort/analysis/plots/communication_overhead_facetgrid.png)

Finally, we will look at the communication overhead of the program. This graph shows the percentage of time spent in
communication portions of the program, broken down by input size along the y axis, processor count along the x axis for
each graph, and input type along the top. The interesting thing to note about this graph is the relationship between
increasing processes and increasing input size. As we increase process counts for smaller input sizes, we see that the
communication overhead increases at a fairly shallow slope, with some increased variation among the larger process counts
likely due to having to communicate across more nodes. However, for the larger input sizes (at the bottom of the graph),
we see a much steeper slope as numer of processes increases. This indicates that the communication overhead is increasing
faster when input size is larger than when input size is smaller. This is likely due to the fact that the larger input
sizes naturally require more networking to communicate, and thus the overhead of parallelization is more pronounced.

In summary, here we see two distinct trends in communication overhead. As process size increases, communication overhead
increases as part of synchronizing all process with MPI_Barrier. As input size increases, communication overhead
increases as part of sending more traffic across the network. These two factors combine as we increase both process size
and input count, resulting in the trends that we see on this graph. We can further validate this by looking at what
percent of communication is spent in MPI_Barrier.

##### MPI Barrier Overhead
![mpi_barrier_percentage_facetgrid.png](bitonic_sort/analysis/plots/mpi_barrier_percentage_facetgrid.png)

This graph shows the percentage of communication time that is taken up by MPI_Barrier waiting to synchronize all
processes. This graph is notably distinct from the last in that we see a much more pronounced increase in MPI_Barrier
time as we increase process count, but a decrease in MPI_Barrier time as we increase input size. This is likely due to
the fact that MPI_Barrier is a blocking call that waits for all processes to reach the same point in the program, and
thus as we increase process count, we increase the time spent waiting for all processes to reach the barrier. However,
again, as we increase input size, we increase the amount of data that needs to be communicated, and thus the time spent
waiting for all processes to reach the barrier decreases as a percentage of total communication time. This graph seems
to provide evidence to the hypothesis that increased process count correlates to increased MPI_Barrier times and
increased input sizes correlates to increase sending and receiving times.

#### Conclusion
In conclusion, we see that bitonic sort has diminishing returns as we increase process count, but still provides a
significant decrease in time per rank as we increase process count. We also see that the overhead of parallelization
increases as we increase input size, but that the benefits of parallelization still outweigh the costs. Additionally,
the initial sorting of the input data does not have a significant effect on the overall performance of the program.
The communication overhead of the program increases as we increase process count and input size, but increasing
process count increases MPI_Barrier time and increasing input size increases sending and receiving time.

## 5. Presentation
Plots for the presentation should be as follows:
- For each implementation:
    - For each of comp_large, comm, and main:
        - Strong scaling plots for each input_size with lines for input_type (7 plots - 4 lines each)
        - Strong scaling speedup plot for each input_type (4 plots)
        - Weak scaling plots for each input_type (4 plots)

Analyze these plots and choose a subset to present and explain in your presentation.

## 6. Final Report
Submit a zip named `TeamX.zip` where `X` is your team number. The zip should contain the following files:
- Algorithms: Directory of source code of your algorithms.
- Data: All `.cali` files used to generate the plots seperated by algorithm/implementation.
- Jupyter notebook: The Jupyter notebook(s) used to generate the plots for the report.
- Report.md

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
p = Number of Processes
array = Array for Problem Type
n = Problem Size

id = Process Identification

master_process = 0
worker_processes = [1, 2, 3, ..., p - 1]

if id is master_process:

    b = n / p

    for index, process in worker_processes:
        start = index * b;
        end = (index + 1) * b
        Send(array[start:end], to=process)

    candidates = []
    for process in worker_processes:
        candidate = Receive(from=process)
        candidates.append(candidate)
    candidates = Sort(candidates)

    splitters = ChooseSplitters(candidates, count=p-1)
    for process in worker_processes:
        Send(splitters, to=worker_processes)

    sample_sort = Receive(from=p-1)

else if id is in worker_process:

    segment = Receive(from=master_process)
    segment = Sort(segment)

    candidates = ChooseCandidates(segment, count=p-1)
    Send(candidates, to=master_process)

    splitters = Receive(from=master_process)

    buckets = [[] * (size(splitters) + 1)]
    for element in segment:
        found = false
        for index, splitter in splitters:
            if element < splitters:
                buckets[index].append(element)
                found = true
                break
        if not found:
            buckets[-1].append(element)

    for index, bucket in buckets:
        buckets[index] = Sort(bucket)

    merged_buckets = []
    if id % 2 == 0:
        Send(buckets, to=process_id + 1)
        received_buckets = Receive(from=process_id - 1)
        merged_buckets = Merge(buckets, received_buckets)
        Send(merged_buckets, to=process_id - 1)
    else if id % 2 == 1:
        received_buckets = Receive(from=process_id - 1)
        merged_buckets = Merge(buckets, received_buckets)
        Send(merged_buckets, to=process_id + 1)

    if id == p - 1:
        Send(merged_buckets, to=master_process)

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

They will show up in the `Thicket.metadata` if the caliper file is read into Thicket.

### **See the `Builds/` directory to find the correct Caliper configurations to get the performance metrics.** They will show up in the `Thicket.dataframe` when the Caliper file is read into Thicket.
## 4. Performance evaluation

# Radix Sort

Sporadic results were observed across all experiments with radix_sort. Illustrating this phenomenon below are the total execution times for radix sort with each type of input data (Sorted input, randomized input, reverse sorted input, and 1 percent perturbed):
![image](radix_sort/plots/radixSortedTotalTime.png)
![image](radix_sort/plots/RadixSortedTotalTimeRandomized.png)
![image](radix_sort/plots/radixSortTotalTimeReverseSorted.png)
![image](radix_sort/plots/radixSortTotalTime1p_perturbed.png)

What can be concluded from these observations is that in the case of radix sort, communication overhead and uneven distribution of work among processes due to the nature of sorting by each digit are the primary time sinks in execution time. The best performance was typically observed among inputs sorted among 2^4 processors. There are diminishing returns for saved exection time after increasing the processors beyond this, as communication overhead between the processes becomes more time consuming.



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

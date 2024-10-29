"""
generate shell for job submission
"""
# example: sbatch 2.grace_job 65536 Random
sort_types = ['Random', '1_perc_perturbed', 'Sorted', 'ReverseSorted']
input_sizes = [2**i for i in range(16, 29, 2)]
num_cores = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]

with open('output.txt', 'a') as f:
    for input_size in input_sizes: 
        for sort_type in sort_types:
            for core in num_cores:
                f.write(f'sbatch {core}.grace_job {input_size} {sort_type}\n')
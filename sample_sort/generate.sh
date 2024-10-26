#!/bin/bash

num_procs=(1 2 4 8 16 32 64 128 256 512 1024)
input_sizes=(16 18 20 22 24 26 28) 
input_types=(0 1 2 3)

output_file="commands.sh"
echo "#!/bin/bash" > "$output_file"

for size in "${input_sizes[@]}"; do
    for type_index in "${!input_types[@]}"; do
        type="${input_types[$type_index]}"
        for proc in "${num_procs[@]}"; do
            if (( proc / 32 >= 2 )); then
                nodes=$(($proc / 32)) 
                echo "sbatch mpi.grace_job $size $proc $type_index --nodes=$nodes --ntasks-per-node=32" >> "$output_file"
            else
                echo "sbatch mpi.grace_job $size $proc $type_index --nodes=1 --ntasks-per-node=$proc" >> "$output_file"
            fi
        done
    done
done

chmod +x "$output_file"

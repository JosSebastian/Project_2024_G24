#!/bin/bash

for exp in {16..28..2}; do
  x=$((2**exp))

  for z in Sorted Random 1_perc_perturbed ReverseSorted; do
    sbatch 2.grace_job "$x" 2 "$z"
  done
done

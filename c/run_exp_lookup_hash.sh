#!/bin/bash

# Define the number of iterations
ITERATIONS=10
START_ITERATION_NUM=0

# Define the arguments for A and B
PAIR_SIZE="16"

ARG_A=""
ARG_B=""
RUN_NUM=0

# SIZES=("1" "10" "100" "1000" "10000")
SIZES=("1" "2" "4" "8" "16" "32" "64" "128" "256" "512" "1024" "2048" "4096" "8192")


# Function to run the binaries
run_binaries() {
    echo "Running A with argument: $ARG_A"
    ./bin/al_sim_device $ARG_A 2> "../experiments/results_lookup_hash_pc/al_results-lookup-hash-${PAIR_SIZE}b_s${RUN_NUM}.json" &
    PID_A=$!

    sleep 1

    echo "Running B with argument: $ARG_B"
    ./bin/al_sim_device $ARG_B 2> "../experiments/results_lookup_hash_pc/al_results-lookup-hash-${PAIR_SIZE}b_r${RUN_NUM}.json" &
    PID_B=$!

    sleep 5

    echo "Running C"
    ./bin/al_sim_upperlayer 10000 1 2
    STATUS=$?
    
    sleep 1

    # Send SIGINT to A and B after C finishes
    # echo "Sending SIGINT to process A (PID: $PID_A)"
    # kill -SIGINT $PID_A

    # echo "Sending SIGINT to process B (PID: $PID_B)"
    # kill -SIGINT $PID_B

    # Wait for A and B to finish
    wait $PID_A
    sleep 2
    wait $PID_B
    sleep 2

    return $STATUS
}

# Loop to execute the binaries ITERATIONS times
for ((i=START_ITERATION_NUM; i<ITERATIONS; i++)); do
    sleep 1

    # Set arguments for each iteration
    
    for j in "${SIZES[@]}"; do
    # Launch the process with each argument
        echo "Starting iteration $i with $j pairs"

        RUN_NUM=$i
        PAIR_SIZE=$j

        ARG_A="1 ${j} 2"
        ARG_B="2 ${j} 1"
        run_binaries

    done


    # Check if C exited normally
    if [ $? -ne 0 ]; then
        echo "C exited with a non-zero status, stopping execution."
        break
    fi
done

echo "Finished all iterations or C exited abnormally."

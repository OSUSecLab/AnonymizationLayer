#!/bin/bash

# Define the number of iterations
ITERATIONS=10

# Define the arguments for A and B
DATA_SIZE="16"

ARG_A=""
ARG_B=""

# Function to run the binaries
run_binaries() {
    echo "Running A with argument: $ARG_A"
    ./bin/al_sim_device $ARG_A 2> "../experiments/results/al_results-lookup-${DATA_SIZE}b_s${i}.json" &
    PID_A=$!

    sleep 1

    echo "Running B with argument: $ARG_B"
    ./bin/al_sim_device $ARG_B 2> "../experiments/results/al_results-lookup-${DATA_SIZE}b_r${i}.json" &
    PID_B=$!

    sleep 1

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
    sleep 1
    wait $PID_B
    sleep 1

    return $STATUS
}

# Loop to execute the binaries ITERATIONS times
for ((i=0; i<ITERATIONS; i++)); do
    echo "Starting iteration $i"
    sleep 1

    # Set arguments for each iteration
    ARG_A="1 1 2"
    ARG_B="2 1 1"

    run_binaries

    # Check if C exited normally
    if [ $? -ne 0 ]; then
        echo "C exited with a non-zero status, stopping execution."
        break
    fi
done

echo "Finished all iterations or C exited abnormally."

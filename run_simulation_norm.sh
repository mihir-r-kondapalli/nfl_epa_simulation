#!/bin/bash

# Flags
QUIET_MODE=false
FETCH_DATA=false

# Parse optional flags
while [[ "$1" == -* ]]; do
    case "$1" in
        -q) QUIET_MODE=true ;;
        -d) FETCH_DATA=true ;;
        *) echo "Unknown flag: $1"; exit 1 ;;
    esac
    shift
done

# Parse positional arguments: [arg0] [iterations]
if [ "$FETCH_DATA" = true ]; then
    if [ "$#" -eq 0 ]; then
        arg0=20
        iterations=3
    elif [ "$#" -eq 1 ]; then
        arg0=$1
        iterations=3
    elif [ "$#" -eq 2 ]; then
        arg0=$1
        iterations=$2
    else
        echo "Usage: $0 [-q] [-d] [arg0] [iterations]"
        exit 1
    fi
else
    if [ "$#" -eq 0 ]; then
        arg0=20
        iterations=3
    elif [ "$#" -eq 1 ]; then
        arg0=20
        iterations=$1
    elif [ "$#" -eq 2 ]; then
        arg0=$1
        iterations=$2
    else
        echo "Usage: $0 [-q] [arg0] [iterations]"
        exit 1
    fi
fi

# Function to run a command with progress display
run_command() {
    local cmd="$1"
    local desc="$2"

    echo ""
    echo -n "$desc..."
    echo ""

    START_TIME=$(date +%s)

    if [ "$QUIET_MODE" = true ]; then
        eval "$cmd" 2>&1 | while read -r line; do
            if [[ "$line" =~ ^[0-9] ]]; then
                echo -ne "\r$line  "
            fi
        done
        END_TIME=$(date +%s)
        RUNTIME=$((END_TIME - START_TIME))
        echo -ne "\r$desc completed in $RUNTIME seconds.\n"
    else
        eval "$cmd"
    fi
}

# Optionally retrieve data
if [ "$FETCH_DATA" = true ]; then
    run_command "Rscript data.R \"$arg0\"" "Processing data"
    run_command "Rscript cdf.R \"$arg0\"" "Generating CDFs"
fi

# Ensure the naive output file exists
if [ ! -f ep_data/norm_eps/naive_eps.csv ]; then
    echo "Error: naive simulation did not produce eps.csv"
    exit 1
fi

# Copy naive eps.csv to temp
cp ep_data/norm_eps/naive_eps.csv ep_data/norm_eps/temp_eps.csv

# Run iterative simulation
for ((i=1; i<iterations; i++)); do
    run_command "./executables/simulator_norm.out ep_data/norm_eps/temp_eps.csv ep_data/norm_eps/temp_eps.csv aux_data/punt_net_yards.json cdf_data aux_data/nfl_fallback_counts.csv" "Running epoch $i"
done

# Final simulation
run_command "./executables/simulator_norm.out ep_data/norm_eps/temp_eps.csv ep_data/norm_eps/final_eps.csv  aux_data/punt_net_yards.json cdf_data aux_data/nfl_fallback_counts.csv" "Running final epoch ($iterations)"

# Final check
if [ -f ep_data/norm_eps/final_eps.csv ]; then
    echo "Simulation completed successfully after $iterations iterations."
else
    echo "Error: Final simulation did not produce eps.csv"
    exit 1
fi

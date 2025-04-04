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

# Default arg0 if not supplied
if [ "$#" -eq 0 ]; then
    arg0=10
elif [ "$#" -eq 1 ]; then
    arg0=$1
else
    echo "Usage: $0 [-q] [-d] [arg0]"
    echo "-q: Quiet mode"
    echo "-d: Fetch/refresh data before simulation"
    echo "arg0: minimum elements per bin (default: 10)"
    exit 1
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
        echo -ne "\r$desc completed in $RUNTIME seconds.\n"
    else
        eval "$cmd"
    fi
}

# Optionally fetch data
if [ "$FETCH_DATA" = true ]; then
    run_command "Rscript rscripts/data.R \"$arg0\"" "Processing data"
    run_command "Rscript rscripts/cdf.R \"$arg0\"" "Generating CDFs"
fi

# Create output directory
mkdir -p ep_data/norm_eps

# Run naive simulation (hardcoded executable + inputs)
run_command "./executables/simulator_naive_norm.out ep_data/norm_eps/naive_eps.csv cdf_data aux_data/nfl_fallback_counts.csv" "Running naive simulation (epoch 0)"

# Confirm output
if [ -f ep_data/norm_eps/naive_eps.csv ]; then
    echo "Simulation completed successfully."
else
    echo "Error: Simulation did not produce naive_eps.csv"
    exit 1
fi

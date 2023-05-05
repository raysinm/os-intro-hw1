#!/bin/bash

# Find the location of the script and change to that directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# Compile the smash executable if it doesn't already exist
if [ ! -f smash ]; then
    make smash
fi

# Run the executable with valgrind for each input file
for input_file in test_input1*.txt; do
    output_file=$(echo $input_file | sed 's/input/output/g')
    valgrind ./smash < "$input_file" > "$output_file"
done
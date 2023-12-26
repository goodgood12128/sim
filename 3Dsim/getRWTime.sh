#!/bin/bash

directory="/home/zmr/3Dsim/just_normal_and_multi-plane/static_allocation_3/"

for file in "$directory"*;do
    filename="$file"
    read_time=$(awk '/read request average response time:/ {print $NF}' "$filename")
    write_time=$(awk '/write request average response time:/ {print $NF}' "$filename")
    read_time=${read_time:-0}
    write_time=${write_time:-0}
    # echo "filename:$(basename "$filename"), read average time: $read_time, write average time: $write_time"
    echo "$(basename "$filename") $read_time $write_time"

    # read_hit=$(awk '/buffer read hits: / {print $NF}' "$filename")
    # read_miss=$(awk '/buffer read miss:/ {print $NF}' "$filename")
    # write_hit=$(awk '/buffer write hits: / {print $NF}' "$filename")
    # write_miss=$(awk '/buffer write miss:/ {print $NF}' "$filename")
#       echo "filename:$(basename "$filename"), read average time: $read_time, write average time: $write_time"
    # echo "$(basename "$filename") $read_hit $read_miss $write_hit $write_miss"
done



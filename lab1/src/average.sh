#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Количество: 0"
    echo "Среднее: 0"
    exit 0
fi

count=$#

sum=0
for num in "$@"; do
    sum=$(echo "$sum + $num" | bc)
done

average=$(echo "scale=2; $sum / $count" | bc)

echo "Количество: $count"
echo "Среднее: $average"

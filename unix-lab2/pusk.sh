#!/bin/sh

for i in $(seq 1 $2); do
    docker run -d --rm --name "Container$i" -v $(pwd)/shared:/shared $1
done

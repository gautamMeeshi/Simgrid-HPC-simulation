#!/bin/bash

scheduler="    <argument value=\"$1\"/><!--scheduler_type-->"
jobs="    <argument value=\"./input/jobs/$2\"/><!--job_file-->"
scheduler_pattern="<!--scheduler_type-->"
jobs_pattern="<!--job_file-->"
deployment_file="input/deployment-5.5.6.2-torus.xml"
temp_file="temp_deployment.xml"

# Using sed to find the pattern and replace the entire line
sed "s|.*$scheduler_pattern.*|$scheduler|" "$deployment_file" > "$temp_file" && mv "$temp_file" "$deployment_file"
sed "s|.*$jobs_pattern.*|$jobs|" "$deployment_file" > "$temp_file" && mv "$temp_file" "$deployment_file"

if [[ "$1" == "remote"* ]]; then
    echo "Starting the python ML program in background"
    python3 src/ml_scheduler.py &
    sleep 6
fi
./exec input/platform-5.5.6.2-torus.xml input/deployment-5.5.6.2-torus.xml
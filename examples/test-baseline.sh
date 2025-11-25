#!/bin/bash
# Example: Baseline Performance Testing

STORAGE="/mnt/storage"
PROFILE="FULLHD-24bit"
THREADS=4
FRAMES=500

echo "Running baseline test..."
vframetest -c -w $PROFILE -t $THREADS -n $FRAMES $STORAGE > baseline-$(date +%Y%m%d).csv

echo "Test complete. Results saved to baseline-$(date +%Y%m%d).csv"

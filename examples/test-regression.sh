#!/bin/bash
# Example: Performance Regression Detection

STORAGE="/mnt/storage"
PROFILE="FULLHD-24bit"
THREADS=2
FRAMES=500

# Save baseline
vframetest -c -w $PROFILE -t $THREADS -n $FRAMES $STORAGE > /tmp/baseline.csv

# Run current test (after changes)
vframetest -c -w $PROFILE -t $THREADS -n $FRAMES $STORAGE > /tmp/current.csv

# Compare (simple diff, could be more sophisticated)
echo "=== Regression Analysis ==="
if diff /tmp/baseline.csv /tmp/current.csv > /dev/null; then
  echo "✅ No performance regression detected"
else
  echo "⚠️  Performance change detected:"
  diff /tmp/baseline.csv /tmp/current.csv
fi

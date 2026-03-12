#! /bin/bash
set -e

# Cleanup function to handle script termination
# This function will be called on script exit or interruption
cleanup() {
    pkill -P $$  # Kill all the child processes of the current process group
    # Optional: Delete temporary files
    [ -f "$TMP_FILE" ] && rm "$TMP_FILE"
    exit 1
}

# Register Signal Capture
trap 'cleanup' INT TERM EXIT


ns=(12)
dims=(8)
deltas=(16 64 256 1024)

for n in "${ns[@]}"; do
  for dim in "${dims[@]}"; do
    for delta in "${deltas[@]}"; do
      ./build/fpsi -d $dim -delta $delta -nn $n -p 0 -prefix
      ./build/fpsi -d $dim -delta $delta -nn $n -p 1 -prefix 
      ./build/fpsi -d $dim -delta $delta -nn $n -p 2 -prefix 
      echo
    done
  done
done



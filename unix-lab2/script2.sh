#!/bin/sh

CONTAINER_ID=$(hostname)
SHARED_DIR="/shared"
LOCK_FILE="$SHARED_DIR/.lock"

mkdir -p "$SHARED_DIR"
touch "$LOCK_FILE"
counter=0

while true; do
  {
       flock -x 9
       i=1
       while [ $i -le 999 ]; do
           free_name=$(printf "%03d" $i)
           if [ ! -e "$SHARED_DIR/$free_name" ]; then
              counter=$((counter + 1))
              tmp="$SHARED_DIR/$free_name.tmp"
              echo "Container: $CONTAINER_ID, File: $counter" > "$tmp"
              if mv "$tmp" "$SHARED_DIR/$free_name"; then
                 filename="$free_name"
                 echo "[$CONTAINER_ID] Created: $filename"
                 break
              else
                 echo "Can't move $tmp to $SHARED_DIR/$free_name"
                 rm -f "$tmp"
              fi
           fi
           i=$((i+1))
       done
   } 9> "$LOCK_FILE"
   if [ -z "$filename" ]; then
      echo "[$CONTAINER_ID] No free filename found" >&2
      sleep 1
      continue
   fi
   sleep 1

   if [ -f "$SHARED_DIR/$filename" ]; then
      rm -f "$SHARED_DIR/$filename"
      echo "[$CONTAINER_ID] Deleted: $filename"
   else
      echo "[$CONTAINER_ID] Error deleting: $filename" >&2
   fi
   sleep 1
done

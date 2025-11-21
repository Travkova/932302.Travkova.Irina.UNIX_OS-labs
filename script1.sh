#!/bin/sh -e

TEMP_DIR=$(mktemp -d)
CURR_DIR=$PWD
FILE=$1

exit_handler() {
    local rc=$?
    trap - EXIT
    rm -rf -- $TEMP_DIR
    exit $rc
}
trap exit_handler EXIT HUP INT QUIT PIPE TERM

if [ ! -e "$FILE" ]; then
   echo "File '$FILE' doesn't exist" >&2
   exit 1
fi

cd "$TEMP_DIR"
FILE_NAME=$(grep '&Output:' "$CURR_DIR/$FILE" | cut -d: -f2)
if [ -z "$FILE_NAME" ]; then
   echo "Can't find comment '&Output' ">&2
   exit 2
fi
if ! g++ "$CURR_DIR/$FILE" -o "$FILE_NAME"; then
    echo "Compilation of File '$FILE' is failed">&2
    exit 3
fi
mv "$FILE_NAME" "$CURR_DIR"
echo "File was assembled in a temporary directory: $TEMP_DIR"

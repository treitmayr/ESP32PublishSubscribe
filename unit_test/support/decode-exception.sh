#!/bin/bash

# Example backtrace:
# Backtrace:0x4008439a:0x3ffc4e400x4008ad85:0x3ffc4e60 0x400911ea:0x3ffc4e80 0x4008ad7b:0x3ffc4ef0 0x400d39f5:0x3ffc4f10 0x400d3a22:0x3ffc4f50 0x400dcf00:0x3ffc4f80

elf_image="$1"
addr2line="${2:-$HOME/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line}"

re_backtrace='^Backtrace:(( ?0x[0-9a-fA-F]{8}:0x[0-9a-fA-F]{8})+)'
re_addr_stack='^ ?(0x[0-9a-fA-F]{8}):0x[0-9a-fA-F]{8} ?'

while IFS= read -r line; do
    echo "$line"
    if [[ "$line" =~ $re_backtrace ]]; then
        addr_str="${BASH_REMATCH[1]}"
        declare -a addrs
        while [ "$addr_str" ]; do
            if [[ "$addr_str" =~ $re_addr_stack ]]; then
                addrs+=("${BASH_REMATCH[1]}")
                match="${BASH_REMATCH[0]}"
                addr_str="${addr_str:${#match}}"
            else
                break
            fi
        done
        if [ ${#addrs} -gt 0 ]; then
            "$addr2line" -afipC -e "$elf_image" "${addrs[@]}" | cat -n
        fi
    fi
done

#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<'EOF'
Usage:
  ./flash.sh <file.elf>
  ./flash.sh <file.bin> [flash_address]

Defaults:
  flash_address for .bin: 0x08000000
EOF
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 1
fi

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

file_path="$1"
flash_address="${2:-0x08000000}"

if ! command -v STM32_Programmer_CLI >/dev/null 2>&1; then
    echo "Error: STM32_Programmer_CLI not found in PATH." >&2
    exit 1
fi

if [[ ! -f "${file_path}" ]]; then
    echo "Error: file not found: ${file_path}" >&2
    exit 1
fi

case "${file_path}" in
    *.elf)
        STM32_Programmer_CLI -c port=SWD -w "${file_path}" -v -rst
        ;;
    *.bin)
        STM32_Programmer_CLI -c port=SWD -w "${file_path}" "${flash_address}" -v -rst
        ;;
    *)
        echo "Error: unsupported file type: ${file_path}" >&2
        echo "Use .elf or .bin." >&2
        exit 1
        ;;
esac

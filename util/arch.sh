#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -e "$DIR/../.arch" ]; then
	cat "$DIR/../.arch"
	exit 0
fi

printf '%s\n' "x86_64"

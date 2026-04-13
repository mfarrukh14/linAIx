#!/bin/bash
set -e
export PATH=/root/gcc_local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export LD_LIBRARY_PATH=/root/gcc_local/x86_64-pc-linux-gnu/x86_64-pc-toaru/lib:/root/gcc_local/lib
cp -a /src /tmp/lb
cd /tmp/lb
# Fix any CRLF from Windows
find . -path ./.claude -prune -o -type f \( -name '*.c' -o -name '*.h' -o -name '*.S' -o -name '*.sh' -o -name '*.krk' -o -name '*.conf' -o -name '*.menu' -o -name 'Makefile' -o -name '*.mk' -o -name '*.ld' -o -name '.yutanirc' -o -name '.eshrc' -o -name '*.launcher' -o -name 'passwd' -o -name 'master.passwd' -o -name 'group' -o -name 'hostname' -o -name 'motd' -o -name 'sudoers' -o -name 'make-version' -o -name '*.trt' -o -name '*.py' \) -print0 | xargs -0 sed -i 's/\r$//'
rm -rf .make
ln -sfn /tmp/lb/base/usr/include/toaru base/usr/include/linAIx
ln -sfn /tmp/lb /root/misaka
mkdir -p util/local/x86_64-pc-toaru/lib
cp /root/gcc_local/x86_64-pc-toaru/lib/libgcc_s.so* util/local/x86_64-pc-toaru/lib/
make ARCH=x86_64 TARGET=x86_64-pc-toaru ARCH_USER_CFLAGS=-D__linAIx__ -j4 2>&1 | tail -30
cp image.iso /src/image.iso
echo BUILD_SUCCESS

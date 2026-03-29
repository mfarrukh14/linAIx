#!/bin/bash
set -e
if [ -d /tmp/toaruos-build ]; then
  docker run --rm -v /tmp/toaruos-build:/d toaruos/build-tools:1.99.x rm -rf /d/.make /d/fatbase /d/base/mod /d/base/lib /d/base/usr /d/base/bin /d/cdrom /d/boot /d/linker /d/image.iso /d/image.dat /d/ramdisk.igz /d/misaka-kernel 2>/dev/null || true
  rm -rf /tmp/toaruos-build
fi
mkdir -p /tmp/toaruos-build
cp -a /mnt/c/Users/Pc/Desktop/MyArchive/code/toaruos/. /tmp/toaruos-build/
cd /tmp/toaruos-build
find . -type f \( -name '*.sh' -o -name '*.krk' -o -name '*.launcher' -o -name '.eshrc' -o -name '.yutanirc' -o -name '.wallpaper.conf' -o -name '.bim3rc' \) -print | xargs perl -pi -e 's/\r\n/\n/g'
find ./base/etc -type f | xargs perl -pi -e 's/\r\n/\n/g'
ln -sfn toaru base/usr/include/linAIx
[ -f apps/toaru_logo.h ] && ln -sf toaru_logo.h apps/linAIx_logo.h
[ -f lib/rline.h ] || ln -s ../base/usr/include/toaru/rline.h lib/rline.h
perl -0pi -e 's/#ifndef __linAIx__/#if 0/g' libc/stdlib/realpath.c
chmod +x base/home/local/.yutanirc base/home/root/.yutanirc base/etc/startup.d/*.sh
docker run --rm -v /tmp/toaruos-build:/root/misaka toaruos/build-tools:1.99.x bash -c '
  ln -sfn /root/gcc_local /root/misaka/util/local
  export LD_LIBRARY_PATH=/root/gcc_local/lib:/root/gcc_local/x86_64-pc-linux-gnu/x86_64-pc-toaru/lib
  cd /root/misaka && make TARGET=x86_64-pc-toaru
'
cp -f /tmp/toaruos-build/image.iso /tmp/toaruos-build/misaka-kernel /tmp/toaruos-build/ramdisk.igz /mnt/c/Users/Pc/Desktop/MyArchive/code/toaruos/
echo "BUILD DONE"

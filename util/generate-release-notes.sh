#!/bin/bash
VERSION=$(git describe --exact-match --tags)
LAST=$(git describe --abbrev=0 --tags ${VERSION}^)
CHANGELOG=$(git log --pretty=format:%s ${LAST}..HEAD | grep ':' | sed -re 's/([^:]*)\:/- \`\1\`\:/' | sort)
cat <<NOTES
# linAIxOS ${VERSION}

Put a screenshot here.

## What's New in ${VERSION}?

Describe the release here.

## What is linAIxOS?

linAIxOS is a hobbyist, educational operating system for x86-64 PCs, focused primarily on use in virtual machines. It provides a Unix-like environment, complete with a graphical desktop interface, shared libraries, feature-rich terminal emulator, and support for running, GCC, Quake, and several other ports. The core of linAIxOS, provided by the CD images in this release, is built completely from scratch. The bootloader, kernel, drivers, C standard library, and userspace applications are all original software created by the authors, as are the graphical assets.

## Who wrote linAIxOS?

linAIxOS is primarily written by a single maintainer, with several contributions from others. A complete list of contributors is available from [AUTHORS](https://github.com/klange/linAIxos/blob/master/AUTHORS).

## Running linAIxOS

It is recommended that you run linAIxOS in a virtual machine / emulator, for maximum compatibility. linAIxOS's driver support is limited, and running on real "bare metal", while possible, does not provide the most complete experience of the OS's capabilities except on very particular hardware. linAIxOS is regularly tested in VirtualBox, QEMU, and VMWare Player, and can be successfully booted (with poor performance) in Bochs. linAIxOS is intended to run from a live CD, though it is possible to install to a hard disk. Additional details on running linAIxOS in different virtual machines is available [from the README](https://github.com/klange/linAIxos#running-linAIxos).

## Release Files

\`image.iso\` is the standard build of linAIxOS, built by the Github Actions CI workflow. It uses linAIxOS's native bootloaders and should work in most virtual machines using BIOS.

## Changelog
${CHANGELOG}

## Known Issues
- The SMP scheduler is known to have performance issues.
- Several utilities, libc functions, and hardware drivers are missing functionality.
- There are many known security issues with linAIxOS. You should not use linAIxOS in a production environment - it is a hobby project, not a production operating system. If you find security issues in linAIxOS and would like to responsibly report them, please file a regular issue report here on GitHub.
NOTES

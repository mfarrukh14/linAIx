#pragma once
/**
 * The sysfunc interface is deprecated. Anything still using these
 * should be migrated to real system calls. The sysfunc interface
 * exists because it was annoying to add new syscall bindings to
 * newlib, but we're not using newlib anymore, so adding new system
 * calls should be easy.
 */

#include <_cheader.h>

/* Privileged */
#define linAIx_SYS_FUNC_SYNC          3
#define linAIx_SYS_FUNC_LOGHERE       4
#define linAIx_SYS_FUNC_KDEBUG        7
#define linAIx_SYS_FUNC_INSMOD        8

/* Unpriviliged */
#define linAIx_SYS_FUNC_SETHEAP       9
#define linAIx_SYS_FUNC_MMAP         10
#define linAIx_SYS_FUNC_THREADNAME   11
#define linAIx_SYS_FUNC_SETVGACURSOR 13
#define linAIx_SYS_FUNC_SETGSBASE    14
#define linAIx_SYS_FUNC_NPROC        15

/* Experimental */
#define linAIx_SYS_FUNC_CLEARICACHE  42
#define linAIx_SYS_FUNC_MUNMAP       43

_Begin_C_Header
extern int sysfunc(int command, char ** args);
_End_C_Header


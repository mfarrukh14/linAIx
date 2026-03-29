/**
 * @brief Print the number of available processors.
 *
 * @copyright
 * This file is part of linAIxOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2021 K. Lange
 */
#include <stdio.h>
#include <sys/sysfunc.h>

int main(int argc, char * argv[]) {
	printf("%d\n", sysfunc(linAIx_SYS_FUNC_NPROC, NULL));
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../linker/dlfcn.h"

static char *argv0;

FILE * bionic_popen(const char* command, const char* mode)
{
	char *new_command = malloc(strlen(argv0) + strlen(command) + 1 + 1);
	strcpy(new_command, argv0);
	strcat(new_command, " ");
	strcat(new_command, command);
	unsetenv("QEMU_GDB"); // don't ask
	return popen(new_command, mode);
}

#if defined(__i386__)|| defined(__aarch64__)
/* musl doesn't have on_exit, so we have to do this shit...
 * do it on glibc as well for consistency */
static int on_exit_status = -1;
/* if we exit by means other than the CTS calling exit(),
 * count that as a failure */

void bionic_exit(int status) {
	on_exit_status = status;
	exit(status);
}

void exit_with_captured_status(void)
{
	fflush(stdout);
	fflush(stderr);
	_Exit(on_exit_status);
}
#endif

int main(int argc, char **argv)
{
	int ret;

	if (argc < 2) {
		return 1;
	}

	argv0 = argv[0];

	void *handle = bionic_dlopen(argv[1], 0);
	if(!handle) {
		fprintf(stderr, "ERROR: test_runner couldn't load the test executable: %s\n", bionic_dlerror());
		exit(1);
	}

#if defined(__i386__) || defined(__aarch64__)
	/* some desctructor registered by the static libcxx causes segfault, this is *probably* fine
	 * (this has to be called after the dlopen so our function gets called first)
	 * note: not sure what the issue is on x86, but on aarch64 the issue is that libcxx has
	 * shadow call stack enabled (probably since android 9) and we're just lucky that it doesn't
	 * seem to crash in less convenient places (it expects everything is compiled with -ffixed-x18,
	 * which is not true on Linux without androideabi) */
	atexit(exit_with_captured_status);
#endif

	typeof(int (int, char**)) *main_func = bionic_dlsym(handle, "main");
	ret = main_func(argc - 1, argv + 1);
#if defined(__i386__) || defined(__aarch64__)
	_Exit(ret);
#endif
	return ret;
}

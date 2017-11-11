#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>


int main(int argc, char **argv)
{
	pid_t pid, sid;
	unsigned long map_addr;
	unsigned long addr = 0x100000;
	unsigned long size = PAGE_SIZE;
	unsigned long jump_len = (1UL << 21);

	pid = fork();
	if (pid < 0)
	goto error;

	if (pid > 0) {
		printf("sleeper_daemon started at pid %d\n", pid);
		exit(EXIT_SUCCESS);
	}

	sid = setsid();
	if (sid < 0)
		goto error;

	if (chdir("/") < 0)
		goto error;

	sleep(10);  /* give time to hook up vm_inspector */

	while (1) {
		map_addr = (unsigned long) mmap(
			(void *)addr, size, 
			PROT_READ|PROT_WRITE, 
			MAP_SHARED|MAP_ANONYMOUS|MAP_POPULATE, 
			-1, 0);
		printf("map made at addr: %#08lx\n", map_addr);
		addr = map_addr + jump_len;
		sleep(5);
	}
	return 0;
error:
	fprintf(stderr, "error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
}

//tells glibc to unlock all kernel features
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

int container_main(void* arg) {
  printf("-> [Container] Entered container execution flow\n");

  if (sethostname("vessel-box", 10) != 0) {
    perror("sethostname");
    return 1;
  }

  struct utsname uts;
  uname(&uts);
  printf("-> [Container] Current hostname %s\n", uts.nodename);

  //Execute target payload
  char* const argv[] = {"/bin/sh", NULL};
  execvp(argv[0], argv);

  perror("execvp");
  return 1;
}


int main(int argc, char** argv) {
  printf("[Host] Starting vessel runtime..\n");

  char* stack = malloc(STACK_SIZE);
  if (!stack) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  char* stack_top = stack + STACK_SIZE;

  int flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | SIGCHLD;

  pid_t pid = clone(container_main, stack_top, flags, NULL);
  if (pid == -1) {
    perror("clone");
    free(stack);
    exit(EXIT_FAILURE);
  }

  printf("[Host] Container spawned with PID: %d\n", pid);

  waitpid(pid, NULL, 0);
  printf("[Host] Container exited. Cleaning up.\n");

  free(stack);
  return 0;
}

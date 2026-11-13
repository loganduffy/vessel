//tells glibc to get all kernel features (mainly clone function)
#define _GNU_SOURCE

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

int container_main(void* arg) {
  char** cmd = (char**)arg;

  printf("-> [Container] Entered container execution flow\n");

  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
    perror("mount private");
    return 1;
  }

  const char* name = "vessel-box";
  if (sethostname(name, strlen(name)) != 0) {
    perror("sethostname");
    return 1;
  }

  struct utsname uts;
  uname(&uts);
  printf("-> [Container] Current hostname %s\n", uts.nodename);

  execvp(cmd[0], cmd);

  perror("execvp");
  return 1;
}

int main(int argc, char** argv) {
  printf("[Host] Starting vessel runtime..\n");

  char* default_cmd[] = {"/bin/sh", NULL};
  char** cmd = (argc >= 2) ? &argv[1] : default_cmd;

  char* stack = malloc(STACK_SIZE);
  if (!stack) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  char* stack_top = stack + STACK_SIZE;

  int flags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | SIGCHLD;

  pid_t pid = clone(container_main, stack_top, flags, cmd);
  if (pid == -1) {
    perror("clone");
    free(stack);
    exit(EXIT_FAILURE);
  }

  printf("[Host] Container spawned with PID: %d\n", pid);

  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    printf("[Host] Container exited with code %d. Cleaning up.\n",
           WEXITSTATUS(status));
  }

  free(stack);
  return 0;
}

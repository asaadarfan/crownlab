/*
 * TASK: timeout
 *
 * Run a command, but kill it if it takes too long.
 *
 *   timeout ticks cmd [args...]
 *
 * timeout runs cmd with its arguments. If cmd is still running after
 * <ticks> clock ticks (one tick is about 1/10 of a second in qemu), it is
 * killed and timeout prints "timeout: killed <cmd>". If cmd finishes in
 * time, timeout prints nothing and exits with cmd's exit status.
 *
 *   - If exec fails, the child prints "timeout: exec <cmd> failed".
 *   - If ticks is not a positive number, print
 *     "timeout: ticks must be positive".
 *
 * The idea: a process can't wait() for a child AND count ticks at the same
 * time. So fork TWO children: one exec's the command, the other is a
 * "timer" that just calls pause(ticks) and exits. Then wait() for whichever
 * finishes first - wait() returns its pid - and kill() the other one.
 *
 * kill() doesn't remove a process right away: it only marks it, and the
 * process exits the next time it goes into or out of the kernel. You still
 * have to wait() for it, or it stays a zombie.
 *
 * Your solution should be in user/timeout.c.
 * Hints: exec(argv[2], &argv[2]) runs the command, because argv already
 *        ends with 0. pause() is called sleep() in xv6 versions before
 *        2025.
 *
 * Sample commands and expected output:
 *   $ timeout 50 echo hi
 *   hi
 *   $ timeout 10 sleep 100      (needs your sleep)
 *   timeout: killed sleep
 *   $ timeout 100 sleep 10
 *   $                           (sleep finished first: timeout says nothing)
 *   $ timeout 10 nosuchcmd
 *   timeout: exec nosuchcmd failed
 *   $ timeout 0 echo hi
 *   timeout: ticks must be positive
 *   $ timeout 5
 *   usage: timeout ticks cmd [args...]
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int n;
  int cmdpid;
  int timerpid;
  int pid;
  int status;

  if (argc < 3) {
    fprintf(2, "usage: timeout ticks cmd [args...]\n");
    exit(1);
  }
  n = atoi(argv[1]);
  if (n <= 0) {
    fprintf(2, "timeout: ticks must be positive\n");
    exit(1);
  }

  // ---- child 1: the command ----
  cmdpid = fork();
  if (cmdpid < 0) {
    fprintf(2, "timeout: fork failed\n");
    exit(1);
  }
  if (cmdpid == 0) {
    exec(argv[2], &argv[2]);
    fprintf(2, "timeout: exec %s failed\n", argv[2]);
    exit(1);
  }

  // ---- child 2: the timer ----
  timerpid = fork();
  if (timerpid < 0) {
    fprintf(2, "timeout: fork failed\n");
    kill(cmdpid);
    wait(0);
    exit(1);
  }
  if (timerpid == 0) {
    pause(n);
    exit(0);
  }

  // ---- parent: who finishes first? ----
  pid = wait(&status);
  if (pid == timerpid) {
    // time is up, the command is still running
    kill(cmdpid);
    wait(0);
    fprintf(2, "timeout: killed %s\n", argv[2]);
    exit(1);
  }

  // the command finished in time: stop the timer
  kill(timerpid);
  wait(0);
  exit(status);
}

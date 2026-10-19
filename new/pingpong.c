/*
 * TASK: pingpong
 *
 * Write a user program that "ping-pongs" a byte between two processes
 * using two pipes, one for each direction.
 *
 *   1. The parent sends one byte to the child.
 *   2. The child prints "<pid>: received ping" (its own pid),
 *      sends the byte back to the parent, and exits.
 *   3. The parent reads the byte from the child, prints
 *      "<pid>: received pong" (its own pid), and exits.
 *
 * Your solution should be in user/pingpong.c.
 * Hints: use pipe() to create a pipe, fork() to create the child,
 *        read()/write() on the pipe, getpid() to get the pid.
 *
 * Sample commands and expected output (pid numbers may differ):
 *   $ pingpong
 *   4: received ping
 *   3: received pong
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int to_child[2];    // parent writes [1], child reads [0]
  int to_parent[2];   // child writes [1], parent reads [0]
  char byte = 'x';
  int pid;

  if (pipe(to_child) < 0) {
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }
  if (pipe(to_parent) < 0) {
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    fprintf(2, "pingpong: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // ---- child ----
    close(to_child[1]);     // child doesn't write to this pipe
    close(to_parent[0]);    // child doesn't read from this pipe

    read(to_child[0], &byte, 1);           // wait for the ping
    printf("%d: received ping\n", getpid());
    write(to_parent[1], &byte, 1);         // send the pong

    close(to_child[0]);
    close(to_parent[1]);
    exit(0);
  }

  // ---- parent ----
  close(to_child[0]);       // parent doesn't read from this pipe
  close(to_parent[1]);      // parent doesn't write to this pipe

  write(to_child[1], &byte, 1);            // send the ping
  read(to_parent[0], &byte, 1);            // wait for the pong
  printf("%d: received pong\n", getpid());

  close(to_child[1]);
  close(to_parent[0]);
  wait(0);                  // let the child finish
  exit(0);
}
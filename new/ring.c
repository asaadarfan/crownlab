/*
 * TASK: ring
 *
 * Pass a token around a RING of N processes, K times, using pipes.
 *
 *   ring N K        (2 <= N <= 6, K >= 1)
 *
 * The processes are numbered 0 .. N-1. Process 0 is the original process,
 * and processes 1 .. N-1 are its children. Process i sends to process
 * (i+1) % N through its own pipe, so the last process sends back to 0:
 *
 *        0 ---> 1 ---> 2 ---> ... ---> N-1
 *        ^                              |
 *        +------------------------------+
 *
 *   1. Process 0 starts the token by sending the int 0 to process 1.
 *   2. Every time a process receives a value v, it prints
 *      "<i>: got <v>" and sends v+1 to the next process.
 *   3. Each time the token comes back to process 0, one round is done.
 *      After K rounds, process 0 prints "done" instead of sending the
 *      token again.
 *   4. Then the ring must shut down cleanly: process 0 closes its write
 *      end, so process 1's read returns 0; process 1 then closes its own
 *      write end and exits, and so on around the ring. Process 0 waits
 *      for all its children before it exits.
 *
 * Step 4 only works if EVERY process closes EVERY pipe end it doesn't use.
 * A process can have at most 16 open files (NOFILE in kernel/param.h),
 * and N pipes use 2*N of them, which is why N is at most 6.
 *
 * Your solution should be in user/ring.c.
 * Hints: create all N pipes first, then fork. pipe i goes from process i
 *        to process (i+1) % N, so process i reads pipe (i-1+N) % N and
 *        writes pipe i. Only one token exists, so the output is always
 *        in the same order.
 *
 * Sample commands and expected output:
 *   $ ring 3 2
 *   1: got 0
 *   2: got 1
 *   0: got 2
 *   1: got 3
 *   2: got 4
 *   0: got 5
 *   done
 *   $ ring 2 1
 *   1: got 0
 *   0: got 1
 *   done
 *   $ ring 9 1
 *   ring: N must be between 2 and 6
 *   $ ring 3
 *   usage: ring N K
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAXN 6

int p[MAXN][2];     // p[i]: from process i to process (i+1) % N

// Close every pipe end, except the read end of pipe 'rp'
// and the write end of pipe 'wp'.
void
close_others(int n, int rp, int wp)
{
  int i;

  for (i = 0; i < n; i++) {
    if (i != rp) {
      close(p[i][0]);
    }
    if (i != wp) {
      close(p[i][1]);
    }
  }
}

// Process i (1 .. N-1): forward the token until the ring shuts down.
void
member(int i, int n)
{
  int from = (i - 1 + n) % n;   // the pipe we read from
  int v;

  close_others(n, from, i);

  while (1) {
    if (read(p[from][0], &v, sizeof(v)) != sizeof(v)) {
      break;                    // previous process closed: shut down
    }
    printf("%d: got %d\n", i, v);
    v = v + 1;
    write(p[i][1], &v, sizeof(v));
  }

  close(p[from][0]);
  close(p[i][1]);               // passes the shutdown to the next process
  exit(0);
}

int
main(int argc, char *argv[])
{
  int n;
  int k;
  int i;
  int pid;
  int v;
  int rounds = 0;

  if (argc != 3) {
    fprintf(2, "usage: ring N K\n");
    exit(1);
  }
  n = atoi(argv[1]);
  k = atoi(argv[2]);
  if (n < 2 || n > MAXN) {
    fprintf(2, "ring: N must be between 2 and %d\n", MAXN);
    exit(1);
  }
  if (k < 1) {
    fprintf(2, "ring: K must be at least 1\n");
    exit(1);
  }

  for (i = 0; i < n; i++) {
    if (pipe(p[i]) < 0) {
      fprintf(2, "ring: pipe failed\n");
      exit(1);
    }
  }

  for (i = 1; i < n; i++) {
    pid = fork();
    if (pid < 0) {
      fprintf(2, "ring: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      member(i, n);             // never returns
    }
  }

  // ---- process 0: reads pipe n-1, writes pipe 0 ----
  close_others(n, n - 1, 0);

  v = 0;
  write(p[0][1], &v, sizeof(v));          // start the token

  while (1) {
    if (read(p[n - 1][0], &v, sizeof(v)) != sizeof(v)) {
      break;                              // should not happen
    }
    printf("0: got %d\n", v);
    rounds++;
    if (rounds == k) {
      printf("done\n");
      break;
    }
    v = v + 1;
    write(p[0][1], &v, sizeof(v));
  }

  close(p[0][1]);               // start the shutdown around the ring
  close(p[n - 1][0]);
  for (i = 1; i < n; i++) {
    wait(0);
  }
  exit(0);
}

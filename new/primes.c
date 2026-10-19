/*
 * TASK: primes
 *
 * Write a concurrent prime sieve using pipes and fork, following the
 * design by Doug McIlroy:
 *
 *   - The first process feeds the numbers 2 through 35 into a pipe.
 *   - Each process in the pipeline reads the first number from its left
 *     neighbour. That number is a prime: print "prime <n>".
 *   - It then creates a new process to its right (connected by a new pipe)
 *     and passes along only the numbers that are NOT divisible by its prime.
 *   - When a process's left pipe is empty (read returns 0), it closes its
 *     pipes, waits for its right neighbour, and exits.
 *
 * Every process must close the pipe ends it doesn't need, or the program
 * runs out of file descriptors / never finishes.
 *
 * Your solution should be in user/primes.c.
 *
 * Sample commands and expected output:
 *   $ primes
 *   prime 2
 *   prime 3
 *   prime 5
 *   prime 7
 *   prime 11
 *   prime 13
 *   prime 17
 *   prime 19
 *   prime 23
 *   prime 29
 *   prime 31
 *   $
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define LIMIT 35

// One stage of the pipeline. 'left' is the pipe we read numbers from.
void sieve(int left) __attribute__((noreturn));

void
sieve(int left)
{
  int prime;
  int n;
  int r;
  int right[2];
  int pid;

  // The first number we receive is always a prime.
  r = read(left, &prime, sizeof(prime));
  if (r != sizeof(prime)) {
    close(left);             // nothing came: the pipeline is finished
    exit(0);
  }
  printf("prime %d\n", prime);

  if (pipe(right) < 0) {
    fprintf(2, "primes: pipe failed\n");
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    fprintf(2, "primes: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child: becomes the next stage, reading from the new pipe
    close(right[1]);
    close(left);
    sieve(right[0]);
  }

  // parent: pass on every number not divisible by our prime
  close(right[0]);
  while (1) {
    r = read(left, &n, sizeof(n));
    if (r != sizeof(n)) {
      break;
    }
    if (n % prime != 0) {
      write(right[1], &n, sizeof(n));
    }
  }

  close(left);
  close(right[1]);           // tells the next stage "no more numbers"
  wait(0);
  exit(0);
}

int
main(int argc, char *argv[])
{
  int p[2];
  int pid;
  int i;

  if (pipe(p) < 0) {
    fprintf(2, "primes: pipe failed\n");
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    fprintf(2, "primes: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    close(p[1]);
    sieve(p[0]);             // first stage
  }

  // the first process just feeds 2..LIMIT into the pipeline
  close(p[0]);
  for (i = 2; i <= LIMIT; i++) {
    write(p[1], &i, sizeof(i));
  }
  close(p[1]);
  wait(0);
  exit(0);
}
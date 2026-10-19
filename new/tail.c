/*
 * TASK: tail
 *
 * Write a simple version of the UNIX tail program for xv6.
 *
 *   tail [-n N] [file]
 *
 * tail prints the LAST N lines of its input (10 lines if -n is not given).
 * If no file is given, it reads from standard input.
 * If the file cannot be opened, print "tail: cannot open <name>".
 * A last line without a '\n' at the end still counts as a line.
 *
 * This is harder than head: you only know which lines are the last ones
 * after you have reached the end of the input, and standard input can't
 * be read twice. So read EVERYTHING into memory first, then walk
 * backwards from the end counting '\n' characters.
 *
 * You don't know the size of the input in advance, so start with a small
 * buffer from malloc() and, whenever it is full, malloc() one twice as
 * big, memmove() the old data into it and free() the old one
 * (xv6 has no realloc).
 *
 * Your solution should be in user/tail.c.
 *
 * Sample commands and expected output:
 *   $ tail -n 2 README
 *   riscv64-softmmu.  Once they are installed, and in your shell
 *   search path, you can run "make qemu".
 *   $ (echo 1; echo 2; echo 3; echo 4) | tail -n 2
 *   3
 *   4
 *   $ (echo 1; echo 2; echo 3) | tail -n 5
 *   1
 *   2
 *   3
 *   $ cat README | tail -n 1
 *   search path, you can run "make qemu".
 *   $ cat README | sort | tail -n 2     (needs your sort)
 *   xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
 *   xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
 *   $ tail nofile
 *   tail: cannot open nofile
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char *data;        // the whole input
int len = 0;       // bytes used in data
int cap = 0;       // bytes malloc'ed for data

// Read all of fd into 'data', growing it when it is full.
// Returns 0 on success, -1 if out of memory.
int
readall(int fd)
{
  char *bigger;
  int n;

  cap = 512;
  data = malloc(cap);
  if (data == 0) {
    return -1;
  }

  while (1) {
    if (len == cap) {
      // full: get a buffer twice as big and move the data into it
      bigger = malloc(cap * 2);
      if (bigger == 0) {
        return -1;
      }
      memmove(bigger, data, len);
      free(data);
      data = bigger;
      cap = cap * 2;
    }

    n = read(fd, data + len, cap - len);
    if (n <= 0) {
      break;                       // end of input
    }
    len = len + n;
  }
  return 0;
}

// Print the last 'max' lines of what readall() read.
void
printlast(int max)
{
  int i;
  int lines = 0;    // '\n's seen walking backwards
  int start = 0;    // where the output begins (0 = print everything)

  if (max <= 0 || len == 0) {
    return;
  }

  // The '\n' at the very end only ends the last line,
  // so start looking from the byte before it.
  i = len - 1;
  if (data[i] == '\n') {
    i--;
  }

  for (; i >= 0; i--) {
    if (data[i] == '\n') {
      lines++;
      if (lines == max) {
        start = i + 1;             // the wanted lines begin after this '\n'
        break;
      }
    }
  }

  write(1, data + start, len - start);

  // a last line with no '\n' at the end: finish it
  if (data[len - 1] != '\n') {
    write(1, "\n", 1);
  }
}

int
main(int argc, char *argv[])
{
  int max = 10;     // default number of lines
  int first = 1;    // index in argv of the file name
  int fd = 0;       // standard input unless a file is given

  // optional "-n N"
  if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
    max = atoi(argv[2]);
    first = 3;
  }

  if (first < argc) {
    fd = open(argv[first], O_RDONLY);
    if (fd < 0) {
      fprintf(2, "tail: cannot open %s\n", argv[first]);
      exit(1);
    }
  }

  if (readall(fd) < 0) {
    fprintf(2, "tail: out of memory\n");
    exit(1);
  }
  if (fd != 0) {
    close(fd);
  }

  printlast(max);
  exit(0);
}

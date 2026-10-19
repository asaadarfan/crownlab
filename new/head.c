/*
 * TASK: head
 *
 * Write a simple version of the UNIX head program for xv6.
 *
 *   head [-n N] [file ...]
 *
 * head prints the first N lines of each file (10 lines if -n is not given).
 * If no file is given, it reads from standard input.
 * If a file cannot be opened, print "head: cannot open <name>" and
 * continue with the next file.
 *
 * Your solution should be in user/head.c.
 *
 * Sample commands and expected output:
 *   $ head -n 2 README
 *   xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
 *   Version 6 (v6).  xv6 loosely follows the structure and style of v6,
 *   $ head README
 *   (the first 10 lines of README)
 *   $ echo one two | head -n 1
 *   one two
 *   $ head -n 1 nofile
 *   head: cannot open nofile
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];

// Print the first 'max' lines from fd.
void
head(int fd, int max)
{
  int n;
  int i;
  int lines = 0;      // newlines printed so far

  if (max <= 0) {
    return;
  }

  while (1) {
    n = read(fd, buf, sizeof(buf));
    if (n <= 0) {
      return;                     // end of file
    }

    for (i = 0; i < n; i++) {
      write(1, &buf[i], 1);       // print one character
      if (buf[i] == '\n') {
        lines++;
        if (lines == max) {
          return;                 // printed enough lines
        }
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  int max = 10;      // default number of lines
  int first = 1;     // index of the first file name in argv
  int fd;
  int i;

  // optional "-n N"
  if (argc >= 3 && strcmp(argv[1], "-n") == 0) {
    max = atoi(argv[2]);
    first = 3;
  }

  // no file names: read standard input
  if (first >= argc) {
    head(0, max);
    exit(0);
  }

  for (i = first; i < argc; i++) {
    fd = open(argv[i], O_RDONLY);
    if (fd < 0) {
      fprintf(2, "head: cannot open %s\n", argv[i]);
      continue;
    }
    head(fd, max);
    close(fd);
  }
  exit(0);
}
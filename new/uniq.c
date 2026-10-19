/*
 * TASK: uniq
 *
 * Write a simple version of the UNIX uniq program for xv6.
 *
 *   uniq [file]
 *
 * uniq prints its input, but when several IDENTICAL lines come one
 * right after another, it prints that line only once.
 * (Only neighbouring duplicates are removed: "a b a" stays "a b a".)
 * If no file is given, read from standard input.
 * If the file cannot be opened, print "uniq: cannot open <name>".
 *
 * Your solution should be in user/uniq.c. Use strcmp() to compare lines
 * and strcpy() to remember the previous line.
 *
 * Sample commands and expected output:
 *   (xv6 sh runs the commands in ( ) one after another, with ; between them)
 *   $ (echo apple; echo apple; echo banana; echo apple) > f
 *   $ uniq f
 *   apple
 *   banana
 *   apple
 *   $ (echo x; echo x; echo y) | uniq
 *   x
 *   y
 *   $ uniq nofile
 *   uniq: cannot open nofile
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];
char cur[512];     // the line being read right now
char prev[512];    // the last line we printed

void
uniq(int fd)
{
  int n;
  int i;
  int len = 0;       // characters in cur
  int have_prev = 0; // 0 until we've printed the first line

  while (1) {
    n = read(fd, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }

    for (i = 0; i < n; i++) {
      if (buf[i] == '\n') {
        cur[len] = '\0';          // the line is complete
        if (have_prev == 0 || strcmp(cur, prev) != 0) {
          printf("%s\n", cur);    // different from the last one: print it
          strcpy(prev, cur);
          have_prev = 1;
        }
        len = 0;                  // start the next line
      } else if (len < sizeof(cur) - 1) {
        cur[len] = buf[i];
        len++;
      }
    }
  }

  // a last line with no newline at the end
  if (len > 0) {
    cur[len] = '\0';
    if (have_prev == 0 || strcmp(cur, prev) != 0) {
      printf("%s\n", cur);
    }
  }
}

int
main(int argc, char *argv[])
{
  int fd;

  if (argc < 2) {
    uniq(0);
    exit(0);
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "uniq: cannot open %s\n", argv[1]);
    exit(1);
  }
  uniq(fd);
  close(fd);
  exit(0);
}
/*
 * TASK: tee (with -a)
 *
 * Write a simple version of the UNIX tee program for xv6.
 *
 *   tee [-a] [file ...]
 *
 * tee reads its standard input and writes everything it reads BOTH to
 * its standard output AND to every file named on the command line.
 *
 *   - Without -a, each file is created, or emptied if it already exists
 *     (O_CREATE | O_TRUNC).
 *   - With -a ("append"), the old contents of each file are kept and the
 *     new text is added at the end.
 *   - If a file can't be opened, print "tee: cannot open <name>" and go
 *     on with the other files.
 *   - At most 10 files.
 *
 * The catch: xv6 has no O_APPEND and no lseek(). But every open file has
 * an offset that read() moves forward, and write() writes at that same
 * offset. So for -a, open the file O_RDWR and read() it until the end
 * before you write anything.
 *
 * Your solution should be in user/tee.c.
 * Hints: open all the files first and keep the fds in an array, then
 *        loop: read() a block from fd 0 and write() it to fd 1 and to
 *        every file.
 *
 * Sample commands and expected output:
 *   $ echo hello | tee f
 *   hello
 *   $ cat f
 *   hello
 *   $ echo more | tee -a f
 *   more
 *   $ cat f
 *   hello
 *   more
 *   $ echo again | tee f g
 *   again
 *   $ cat f
 *   again
 *   $ cat g
 *   again
 *   $ mkdir d
 *   $ echo x | tee d f
 *   tee: cannot open d
 *   x
 *   $ cat f
 *   x
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXF 10

char buf[512];
int fds[MAXF];      // the files we write to

// Move fd's offset to the end of the file, by reading everything.
void
skip_to_end(int fd)
{
  while (1) {
    if (read(fd, buf, sizeof(buf)) <= 0) {
      break;
    }
  }
}

int
main(int argc, char *argv[])
{
  int append = 0;   // -a
  int first = 1;    // index in argv of the first file name
  int nf = 0;       // how many files are open
  int i;
  int n;
  int fd;

  if (argc > 1 && strcmp(argv[1], "-a") == 0) {
    append = 1;
    first = 2;
  }

  for (i = first; i < argc; i++) {
    if (nf == MAXF) {
      fprintf(2, "tee: too many files\n");
      exit(1);
    }

    if (append) {
      fd = open(argv[i], O_CREATE | O_RDWR);
      if (fd >= 0) {
        skip_to_end(fd);
      }
    } else {
      fd = open(argv[i], O_CREATE | O_WRONLY | O_TRUNC);
    }

    if (fd < 0) {
      // O_CREATE fails on a directory, too
      fprintf(2, "tee: cannot open %s\n", argv[i]);
      continue;
    }
    fds[nf] = fd;
    nf++;
  }

  while (1) {
    n = read(0, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }
    write(1, buf, n);
    for (i = 0; i < nf; i++) {
      write(fds[i], buf, n);
    }
  }

  for (i = 0; i < nf; i++) {
    close(fds[i]);
  }
  exit(0);
}

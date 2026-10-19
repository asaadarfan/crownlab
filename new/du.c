/*
 * TASK: du (disk usage)
 *
 * Write a simple version of the UNIX du program for xv6.
 *
 *   du <path>
 *
 * du walks the directory tree starting at <path> (like find does).
 * For every regular FILE it finds, it prints "<size> <path>", where size
 * is the file's size in bytes (from fstat). At the end it prints
 * "total <sum>", the sum of all the file sizes.
 *
 * Don't recurse into "." and "..". If <path> is a single file, just print
 * that file and the total. If <path> can't be opened, print
 * "du: cannot open <path>".
 *
 * Your solution should be in user/du.c.
 * Hints: look at user/ls.c and your find.c; make the recursive function
 *        RETURN the total size of everything below it.
 *
 * Sample commands and expected output:
 *   $ mkdir d
 *   $ echo hello > d/x        (6 bytes: h e l l o \n)
 *   $ echo hi > d/y           (3 bytes)
 *   $ du d
 *   6 d/x
 *   3 d/y
 *   total 9
 *   $ du d/x
 *   6 d/x
 *   total 6
 *   $ du nodir
 *   du: cannot open nodir
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Print every file under 'path' and return the sum of their sizes.
int
du(char *path)
{
  char buf[512];
  char *p;
  int fd;
  int total = 0;
  struct dirent de;
  struct stat st;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "du: cannot open %s\n", path);
    return 0;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "du: cannot stat %s\n", path);
    close(fd);
    return 0;
  }

  if (st.type == T_FILE) {
    printf("%d %s\n", (int)st.size, path);
    total = st.size;
  }

  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      fprintf(2, "du: path too long\n");
      close(fd);
      return 0;
    }

    // buf = "path/"
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p = '/';
    p++;

    while (1) {
      if (read(fd, &de, sizeof(de)) != sizeof(de)) {
        break;
      }
      if (de.inum == 0) {
        continue;
      }

      // buf = "path/entryname"
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0) {
        continue;
      }

      total = total + du(buf);     // add everything inside this entry
    }
  }

  close(fd);
  return total;
}

int
main(int argc, char *argv[])
{
  int total;
  int fd;

  if (argc != 2) {
    fprintf(2, "usage: du path\n");
    exit(1);
  }

  // check the starting path exists, so we don't print "total 0" for it
  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "du: cannot open %s\n", argv[1]);
    exit(1);
  }
  close(fd);

  total = du(argv[1]);
  printf("total %d\n", total);
  exit(0);
}
/*
 * TASK: rmr (rm -r)
 *
 * xv6's rm can't remove a directory that still has files in it, because
 * unlink() refuses to remove a directory that isn't empty. Write rmr,
 * which removes a file, or a directory together with EVERYTHING inside it.
 *
 *   rmr path [path ...]
 *
 * For a directory, first remove everything inside it (recursively), and
 * only then the directory itself. Print "removed <path>" for every file
 * and directory that is removed, so the things inside a directory are
 * printed before the directory.
 *
 *   - Never go into "." and ".." inside a directory.
 *   - Refuse to remove a path whose last part is "." or "..":
 *     print "rmr: refusing to remove <path>".
 *   - If a path can't be opened, print "rmr: cannot open <path>".
 *   - If unlink fails, print "rmr: cannot remove <path>".
 *   After an error, go on with the remaining paths.
 *
 * Your solution should be in user/rmr.c.
 * Hints: start from your find.c / du.c. You may unlink entries of a
 *        directory while you are still reading it: unlink() just sets
 *        the entry's inum to 0, and you skip entries with inum 0 anyway.
 *        Close the directory before you unlink it.
 *
 * Sample commands and expected output:
 *   $ mkdir t
 *   $ echo > t/a
 *   $ mkdir t/sub
 *   $ echo > t/sub/b
 *   $ echo > t/sub/c
 *   $ rm t
 *   rm: t failed to delete
 *   $ rmr t
 *   removed t/a
 *   removed t/sub/b
 *   removed t/sub/c
 *   removed t/sub
 *   removed t
 *   $ ls t
 *   ls: cannot open t
 *   $ echo > x
 *   $ rmr x nofile
 *   removed x
 *   rmr: cannot open nofile
 *   $ rmr .
 *   rmr: refusing to remove .
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Return the part of the path after the last '/'.
char *
lastname(char *path)
{
  char *p;

  p = path + strlen(path);
  while (p >= path && *p != '/') {
    p--;
  }
  p++;
  return p;
}

// Remove 'path' and everything below it. Returns 0 if everything was
// removed, -1 if something went wrong.
int
rmr(char *path)
{
  char buf[512];
  char *p;
  int fd;
  int failed = 0;
  struct dirent de;
  struct stat st;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "rmr: cannot open %s\n", path);
    return -1;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "rmr: cannot stat %s\n", path);
    close(fd);
    return -1;
  }

  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      fprintf(2, "rmr: path too long\n");
      close(fd);
      return -1;
    }

    // buf = "path/"
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p = '/';
    p++;

    // first empty the directory
    while (1) {
      if (read(fd, &de, sizeof(de)) != sizeof(de)) {
        break;
      }
      if (de.inum == 0) {
        continue;                   // empty slot (or something we removed)
      }

      // buf = "path/entryname"
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0) {
        continue;
      }

      if (rmr(buf) < 0) {
        failed = 1;
      }
    }
  }

  close(fd);

  // If something inside couldn't be removed, the directory isn't empty
  // and unlink would fail anyway.
  if (failed) {
    return -1;
  }

  // now it's a file or an empty directory: remove it
  if (unlink(path) < 0) {
    fprintf(2, "rmr: cannot remove %s\n", path);
    return -1;
  }
  printf("removed %s\n", path);
  return 0;
}

int
main(int argc, char *argv[])
{
  int i;
  char *name;
  int status = 0;

  if (argc < 2) {
    fprintf(2, "usage: rmr path [path ...]\n");
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    name = lastname(argv[i]);
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      fprintf(2, "rmr: refusing to remove %s\n", argv[i]);
      status = 1;
      continue;
    }
    if (rmr(argv[i]) < 0) {
      status = 1;
    }
  }
  exit(status);
}

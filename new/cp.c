/*
 * TASK: cp (with -r)
 *
 * xv6 has no cp. Write one.
 *
 *   cp src dst
 *   cp -r src dst
 *
 *   - If dst is an existing DIRECTORY, the copy goes inside it, with the
 *     same last name as src: "cp a/f d" makes "d/f".
 *     Otherwise the copy is called dst.
 *   - Copying a file: create the target (O_CREATE | O_TRUNC, so an old
 *     file with that name is overwritten) and copy all the bytes.
 *   - Copying a directory needs -r. Without it print
 *     "cp: -r not given; omitting directory <src>".
 *     With -r: mkdir() the target, then copy every entry of src
 *     (except "." and "..") into it, recursively.
 *   - If src can't be opened print "cp: cannot open <src>",
 *     if the target can't be created print "cp: cannot create <target>".
 *
 * You don't have to handle copying a directory into itself (cp -r a a/b).
 *
 * Your solution should be in user/cp.c.
 * Hints: use stat() to find out whether dst is an existing directory.
 *        You need TWO paths while walking the tree: one in the source
 *        tree and one in the copy. Build both the way find.c builds
 *        "path/name". The user stack is small, so don't put big arrays
 *        in a recursive function: the copy buffer can be a global.
 *
 * Sample commands and expected output:
 *   $ echo hello > f
 *   $ cp f g
 *   $ cat g
 *   hello
 *   $ mkdir d
 *   $ cp f d
 *   $ cat d/f
 *   hello
 *   $ mkdir d/e
 *   $ echo deep > d/e/x
 *   $ cp d d2
 *   cp: -r not given; omitting directory d
 *   $ cp -r d d2
 *   $ cat d2/f
 *   hello
 *   $ cat d2/e/x
 *   deep
 *   $ mkdir backup
 *   $ cp -r d backup         (backup exists, so this makes backup/d)
 *   $ cat backup/d/e/x
 *   deep
 *   $ cp README d/e/x
 *   $ wc d/e/x
 *   (same numbers as wc README)
 *   $ cp nofile g
 *   cp: cannot open nofile
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

char buf[512];      // copy buffer (global: keeps the stack small)
int recursive = 0;  // -r

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

// out = dir + "/" + name. Returns -1 if it doesn't fit.
int
join(char *out, char *dir, char *name)
{
  if (strlen(dir) + 1 + strlen(name) + 1 > MAXPATH) {
    fprintf(2, "cp: path too long\n");
    return -1;
  }
  strcpy(out, dir);
  out[strlen(dir)] = '/';
  strcpy(out + strlen(dir) + 1, name);
  return 0;
}

// Copy the bytes of the open file 'in' into a new file called 'to'.
int
copyfile(int in, char *to)
{
  int out;
  int n;

  out = open(to, O_CREATE | O_WRONLY | O_TRUNC);
  if (out < 0) {
    fprintf(2, "cp: cannot create %s\n", to);
    return -1;
  }

  while (1) {
    n = read(in, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }
    if (write(out, buf, n) != n) {
      fprintf(2, "cp: write error on %s\n", to);
      close(out);
      return -1;
    }
  }
  close(out);
  return 0;
}

// Copy 'from' (a file or a directory) to the new name 'to'.
int
copy(char *from, char *to)
{
  char subfrom[MAXPATH];
  char subto[MAXPATH];
  char name[DIRSIZ + 1];
  int fd;
  int failed = 0;
  struct dirent de;
  struct stat st;

  fd = open(from, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "cp: cannot open %s\n", from);
    return -1;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "cp: cannot stat %s\n", from);
    close(fd);
    return -1;
  }

  if (st.type != T_DIR) {
    failed = copyfile(fd, to);
    close(fd);
    return failed;
  }

  // ---- a directory ----
  if (recursive == 0) {
    fprintf(2, "cp: -r not given; omitting directory %s\n", from);
    close(fd);
    return -1;
  }
  if (mkdir(to) < 0) {
    fprintf(2, "cp: cannot create %s\n", to);
    close(fd);
    return -1;
  }

  while (1) {
    if (read(fd, &de, sizeof(de)) != sizeof(de)) {
      break;
    }
    if (de.inum == 0) {
      continue;
    }

    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    // from/name  ->  to/name
    if (join(subfrom, from, name) < 0 || join(subto, to, name) < 0) {
      failed = -1;
      continue;
    }
    if (copy(subfrom, subto) < 0) {
      failed = -1;
    }
  }

  close(fd);
  return failed;
}

int
main(int argc, char *argv[])
{
  char target[MAXPATH];
  char *src;
  char *dst;
  struct stat st;

  if (argc == 4 && strcmp(argv[1], "-r") == 0) {
    recursive = 1;
    src = argv[2];
    dst = argv[3];
  } else if (argc == 3) {
    src = argv[1];
    dst = argv[2];
  } else {
    fprintf(2, "usage: cp [-r] src dst\n");
    exit(1);
  }

  // dst is an existing directory: the copy goes inside it
  if (stat(dst, &st) >= 0 && st.type == T_DIR) {
    if (join(target, dst, lastname(src)) < 0) {
      exit(1);
    }
  } else {
    if (strlen(dst) + 1 > MAXPATH) {
      fprintf(2, "cp: path too long\n");
      exit(1);
    }
    strcpy(target, dst);
  }

  if (copy(src, target) < 0) {
    exit(1);
  }
  exit(0);
}

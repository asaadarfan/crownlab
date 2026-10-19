/*
 * TASK: mv
 *
 * xv6 has no mv. Write one.
 *
 *   mv src dst
 *   mv src [src ...] dir
 *
 * Moving a file doesn't copy any bytes: give the file's inode a new
 * name with link(), then remove the old name with unlink().
 *
 *   - If the last argument is an existing DIRECTORY, every src is moved
 *     into it with the same last name: "mv a/f d" makes "d/f".
 *     Otherwise the new name is dst itself.
 *   - With more than one src, the last argument must be a directory,
 *     otherwise print "mv: target <dst> is not a directory".
 *   - If the new name already exists, it is replaced (unlink it first,
 *     because link() fails when the new name exists).
 *     But if it is the SAME file as src (same inode), print
 *     "mv: <src> and <new name> are the same file" and leave it alone,
 *     or you would unlink the only name the file has!
 *   - xv6's link() refuses directories, so mv only moves files.
 *     For a directory print "mv: cannot move directory <src>".
 *   - If src doesn't exist print "mv: cannot open <src>".
 *   After an error, go on with the remaining srcs.
 *
 * You don't have to handle paths that end in '/'.
 *
 * Your solution should be in user/mv.c.
 * Hints: stat() tells you whether a path exists, whether it is a
 *        directory (st.type == T_DIR), and which inode it is (st.dev and
 *        st.ino). Look at sys_link() in kernel/sysfile.c to see why
 *        directories are refused. Build "dir/name" the way cp.c does.
 *
 * Sample commands and expected output:
 *   $ echo hi > f
 *   $ mv f g
 *   $ cat f
 *   cat: cannot open f
 *   $ cat g
 *   hi
 *   $ mkdir d
 *   $ mv g d
 *   $ cat d/g
 *   hi
 *   $ echo a > x
 *   $ echo b > y
 *   $ mv x y
 *   $ cat y
 *   a
 *   $ echo 1 > p
 *   $ echo 2 > q
 *   $ mv p q d
 *   $ cat d/q
 *   2
 *   $ mv d/g d
 *   mv: d/g and d/g are the same file
 *   $ mv y y
 *   mv: y and y are the same file
 *   $ mv d e
 *   mv: cannot move directory d
 *   $ mv nofile z
 *   mv: cannot open nofile
 *   $ mv d/p d/q y
 *   mv: target y is not a directory
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
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

// out = dir + "/" + name. Returns -1 if it doesn't fit.
int
join(char *out, char *dir, char *name)
{
  if (strlen(dir) + 1 + strlen(name) + 1 > MAXPATH) {
    fprintf(2, "mv: path too long\n");
    return -1;
  }
  strcpy(out, dir);
  out[strlen(dir)] = '/';
  strcpy(out + strlen(dir) + 1, name);
  return 0;
}

// Move src to dst, or into dst if 'into' is 1 (dst is a directory).
int
move(char *src, char *dst, int into)
{
  char path[MAXPATH];
  char *target;
  struct stat st;   // src
  struct stat tst;  // target, if it exists

  if (stat(src, &st) < 0) {
    fprintf(2, "mv: cannot open %s\n", src);
    return -1;
  }
  if (st.type == T_DIR) {
    fprintf(2, "mv: cannot move directory %s\n", src);
    return -1;
  }

  if (into) {
    if (join(path, dst, lastname(src)) < 0) {
      return -1;
    }
    target = path;
  } else {
    target = dst;
  }

  // Does the new name already exist?
  if (stat(target, &tst) >= 0) {
    if (tst.dev == st.dev && tst.ino == st.ino) {
      fprintf(2, "mv: %s and %s are the same file\n", src, target);
      return -1;
    }
    if (tst.type == T_DIR) {
      fprintf(2, "mv: cannot overwrite directory %s\n", target);
      return -1;
    }
    if (unlink(target) < 0) {
      fprintf(2, "mv: cannot remove %s\n", target);
      return -1;
    }
  }

  // new name first, so the file always has at least one name
  if (link(src, target) < 0) {
    fprintf(2, "mv: cannot create %s\n", target);
    return -1;
  }
  if (unlink(src) < 0) {
    fprintf(2, "mv: cannot remove %s\n", src);
    return -1;
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  char *dst;
  int into = 0;
  int failed = 0;
  int i;
  struct stat st;

  if (argc < 3) {
    fprintf(2, "usage: mv src [src ...] dst\n");
    exit(1);
  }

  dst = argv[argc - 1];
  if (stat(dst, &st) >= 0 && st.type == T_DIR) {
    into = 1;
  }
  if (argc > 3 && into == 0) {
    fprintf(2, "mv: target %s is not a directory\n", dst);
    exit(1);
  }

  for (i = 1; i < argc - 1; i++) {
    if (move(argv[i], dst, into) < 0) {
      failed = 1;
    }
  }
  exit(failed);
}

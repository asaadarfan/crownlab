#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

char *
filename(char *path)
{
  char *name = path;
  for (char *p = path; *p != '\0'; p++) {
    if (*p == '/')
      name = p + 1;
  }
  return name;
}

int
match(char *path, char **command, int count)
{
  if (count == 0) {
    printf("%s\n", path);
    return 0;
  }

  char *args[MAXARG];
  for (int i = 0; i < count; i++)
    args[i] = command[i];
  args[count] = path;
  args[count + 1] = 0;

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "find: fork failed\n");
    return 1;
  }
  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "find: cannot exec %s\n", args[0]);
    exit(1);
  }

  int status;
  if (wait(&status) != pid) {
    fprintf(2, "find: wait failed\n");
    return 1;
  }
  return status != 0;
}

int
find(char *path, char *target, char **command, int count)
{
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return 1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return 1;
  }

  int failed = 0;
  if (strcmp(filename(path), target) == 0)
    failed = match(path, command, count);

  if (st.type == T_DIR) {
    char buf[512];
    struct dirent de;
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      fprintf(2, "find: path too long: %s\n", path);
      close(fd);
      return 1;
    }

    strcpy(buf, path);
    char *p = buf + strlen(buf);
    if (p == buf || p[-1] != '/')
      *p++ = '/';

    int n;
    while ((n = read(fd, &de, sizeof(de))) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = '\0';
      if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
        continue;
      if (find(buf, target, command, count) != 0)
        failed = 1;
    }
    if (n != 0) {
      fprintf(2, "find: cannot read directory %s\n", path);
      failed = 1;
    }
  }

  if (close(fd) < 0) {
    fprintf(2, "find: cannot close %s\n", path);
    failed = 1;
  }
  return failed;
}

int
main(int argc, char *argv[])
{
  if (argc < 3 || (argc > 3 &&
      (argc < 5 || strcmp(argv[3], "-exec") != 0))) {
    fprintf(2, "Usage: find path name [-exec command [args...]]\n");
    exit(1);
  }

  int count = argc > 3 ? argc - 4 : 0;
  if (count > MAXARG - 2) {
    fprintf(2, "find: too many command arguments\n");
    exit(1);
  }

  char *path = argv[1];
  int length = strlen(path);
  while (length > 1 && path[length - 1] == '/')
    path[--length] = '\0';

  exit(find(path, argv[2], count ? &argv[4] : 0, count));
}


/*
 * find: search a directory tree for files/folders with a given name.
 *       With -exec, run a command on each match instead of printing it.
 *
 * Usage: find <dir> <name> [-exec <cmd> [args...]]
 *
 * Setup (run inside xv6 after make qemu):
 *   $ echo > b
 *   $ mkdir a
 *   $ echo > a/b
 *   $ mkdir a/aa
 *   $ echo > a/aa/b
 *   $ echo hello > a/hi
 *
 * Without -exec:
 *   $ find . b          -> ./b
 *                          ./a/b
 *                          ./a/aa/b
 *   $ find a b          -> a/b
 *                          a/aa/b
 *   $ find . aa         -> ./a/aa          (directories match too)
 *   $ find . zzz        -> (nothing)       (no matches)
 *   $ find nodir b      -> find: cannot open nodir
 *
 * With -exec (the found path is added as the last argument):
 *   $ find . wc -exec echo hi        -> hi ./wc
 *   $ find . b -exec echo found      -> found ./b
 *                                       found ./a/b
 *                                       found ./a/aa/b
 *   $ find . hi -exec cat            -> hello        (runs: cat ./a/hi)
 *   $ find . hi -exec wc             -> 1 1 6 ./a/hi
 *   $ find . zzz -exec echo hi       -> (nothing)    (command never runs)
 *   $ find . b -exec nosuchcmd       -> find: exec nosuchcmd failed  (x3)
 *
 * Usage errors:
 *   $ find .                         -> usage message
 *   $ find . b -exec                 -> usage message (no command)
 *   $ find . b -foo echo             -> usage message (not -exec)
 *
 * Official test:  $ sh < findtest.sh   (inside xv6)
 *                 $ make grade         (in the Ubuntu terminal)
 */
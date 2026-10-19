#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// Return the part of the path after the last '/'.
// "./a/aa/b" -> "b"
char *
lastname(char *path)
{
  char *p;

  p = path + strlen(path);          // start at the end
  while (p >= path && *p != '/') {  // walk back to the last '/'
    p--;
  }
  p++;                              // step past the '/'
  return p;
}

void
find(char *path, char *name)
{
  char buf[512];
  char *p;
  int fd;
  struct dirent de;
  struct stat st;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  // Does this file or directory itself have the name we want?
  if (strcmp(lastname(path), name) == 0) {
    printf("%s\n", path);
  }

  // If it's a directory, look at everything inside it.
  if (st.type == T_DIR) {
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      fprintf(2, "find: path too long\n");
      close(fd);
      return;
    }

    // buf = "path/"
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p = '/';
    p++;

    while (1) {
      if (read(fd, &de, sizeof(de)) != sizeof(de)) {
        break;                       // no more entries
      }
      if (de.inum == 0) {
        continue;                    // empty slot
      }

      // buf = "path/entryname"
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;                 // make sure the name ends

      if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0) {
        continue;                    // don't go into . or ..
      }

      find(buf, name);               // recursion: search inside it
    }
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(2, "usage: find dir name\n");
    exit(1);
  }

  find(argv[1], argv[2]);
  exit(0);
}
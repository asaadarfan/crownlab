/*
 * TASK: sort
 *
 * Write a simple version of the UNIX sort program for xv6.
 *
 *   sort [-r] [-n] [-u] [file]
 *
 * sort reads all the lines of its input and prints them in increasing
 * order. Options (they can also be combined, e.g. -rn):
 *   -n   compare lines as numbers (with atoi) instead of as strings,
 *        so "10" comes after "9"
 *   -r   reverse: print in decreasing order
 *   -u   unique: print only the first of several lines that compare equal
 *
 * If no file is given, read from standard input.
 * If the file cannot be opened, print "sort: cannot open <name>".
 * For an unknown option print "sort: unknown option -<c>".
 *
 * Plan:
 *   1. Read the whole input into one malloc'ed buffer that you grow
 *      when it is full (see tail).
 *   2. Turn every '\n' into '\0', so every line is a string inside the
 *      buffer, and make an array of pointers to the starts of the lines.
 *   3. Sort the ARRAY OF POINTERS (insertion sort is fine) - the text of
 *      the lines never moves.
 *
 * Your solution should be in user/sort.c.
 *
 * Sample commands and expected output:
 *   $ (echo pear; echo apple; echo fig; echo apple) > f
 *   $ sort f
 *   apple
 *   apple
 *   fig
 *   pear
 *   $ sort -u f
 *   apple
 *   fig
 *   pear
 *   $ sort -r f
 *   pear
 *   fig
 *   apple
 *   apple
 *   $ (echo 10; echo 9; echo 100; echo 7) > nums
 *   $ sort nums
 *   10
 *   100
 *   7
 *   9
 *   $ sort -n nums
 *   7
 *   9
 *   10
 *   100
 *   $ cat nums | sort -rn
 *   100
 *   10
 *   9
 *   7
 *   $ sort -x f
 *   sort: unknown option -x
 *   $ sort nofile
 *   sort: cannot open nofile
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char *data;        // the whole input
int len = 0;       // bytes used in data
int cap = 0;       // bytes malloc'ed for data

int numeric = 0;   // -n
int reverse = 0;   // -r
int unique = 0;    // -u

// Read all of fd into 'data', growing it when it is full.
// Returns 0 on success, -1 if out of memory.
int
readall(int fd)
{
  char *bigger;
  int n;

  cap = 512;
  data = malloc(cap + 1);           // +1 room for a final '\0'
  if (data == 0) {
    return -1;
  }

  while (1) {
    if (len == cap) {
      bigger = malloc(cap * 2 + 1);
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
      break;
    }
    len = len + n;
  }
  return 0;
}

// <0 if a comes before b, 0 if they are equal, >0 if a comes after b
// (ignoring -r).
int
compare(char *a, char *b)
{
  int x;
  int y;

  if (numeric) {
    x = atoi(a);
    y = atoi(b);
    if (x < y) {
      return -1;
    }
    if (x > y) {
      return 1;
    }
    return 0;
  }
  return strcmp(a, b);
}

// Should line a be printed before line b?
int
before(char *a, char *b)
{
  if (reverse) {
    return compare(a, b) > 0;
  }
  return compare(a, b) < 0;
}

int
main(int argc, char *argv[])
{
  char **lines;
  char *key;
  int nlines = 0;
  int fd = 0;
  int i;
  int j;
  char *opt;

  // options: every argument that starts with '-'
  i = 1;
  while (i < argc && argv[i][0] == '-') {
    for (opt = argv[i] + 1; *opt != '\0'; opt++) {
      if (*opt == 'n') {
        numeric = 1;
      } else if (*opt == 'r') {
        reverse = 1;
      } else if (*opt == 'u') {
        unique = 1;
      } else {
        fprintf(2, "sort: unknown option -%c\n", *opt);
        exit(1);
      }
    }
    i++;
  }

  if (i < argc) {
    fd = open(argv[i], O_RDONLY);
    if (fd < 0) {
      fprintf(2, "sort: cannot open %s\n", argv[i]);
      exit(1);
    }
  }

  if (readall(fd) < 0) {
    fprintf(2, "sort: out of memory\n");
    exit(1);
  }
  if (fd != 0) {
    close(fd);
  }
  if (len == 0) {
    exit(0);
  }

  // A last line without '\n' gets one, so every line ends the same way.
  // (readall left room for one more byte.)
  if (data[len - 1] != '\n') {
    data[len] = '\n';
    len++;
  }

  // count the lines, then make the array of pointers
  for (i = 0; i < len; i++) {
    if (data[i] == '\n') {
      nlines++;
    }
  }
  lines = malloc(nlines * sizeof(char *));
  if (lines == 0) {
    fprintf(2, "sort: out of memory\n");
    exit(1);
  }

  j = 0;
  lines[j] = data;                  // the first line starts at the beginning
  j++;
  for (i = 0; i < len; i++) {
    if (data[i] == '\n') {
      data[i] = '\0';               // end this line
      if (j < nlines) {
        lines[j] = &data[i + 1];    // the next line starts right after
        j++;
      }
    }
  }

  // insertion sort on the pointers
  for (i = 1; i < nlines; i++) {
    key = lines[i];
    j = i - 1;
    while (j >= 0 && before(key, lines[j])) {
      lines[j + 1] = lines[j];      // shift the bigger line right
      j--;
    }
    lines[j + 1] = key;
  }

  for (i = 0; i < nlines; i++) {
    // with -u, equal lines are next to each other now: skip repeats
    if (unique && i > 0 && compare(lines[i], lines[i - 1]) == 0) {
      continue;
    }
    printf("%s\n", lines[i]);
  }
  exit(0);
}

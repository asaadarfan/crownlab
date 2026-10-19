/*
 * TASK: xargs
 *
 * Write a simple version of the UNIX xargs program for xv6.
 * xargs takes a command (with optional arguments) on its command line.
 * It reads lines from standard input, and for EACH line it runs the
 * command with that line added as one extra argument at the end.
 *
 *   echo hello too | xargs echo bye
 *     -> runs: echo bye "hello too"
 *     -> prints: bye hello too
 *
 * Use fork, exec and wait for each line. Read standard input one
 * character at a time until a newline. kernel/param.h declares MAXARG,
 * which you can use to size the argv array.
 *
 * Your solution should be in user/xargs.c.
 *
 * Sample commands and expected output:
 *   $ echo hello too | xargs echo bye
 *   bye hello too
 *   $ echo README | xargs wc
 *   48 336 2441 README
 *   $ echo > b
 *   $ mkdir a
 *   $ echo > a/b
 *   $ find . b | xargs echo found        (needs your find program)
 *   found ./b
 *   found ./a/b
 *   $ xargs
 *   usage: xargs command [args...]
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

// Run argv[0] with the given argument list, and wait for it.
void
run(char *args[])
{
  int pid;

  pid = fork();
  if (pid < 0) {
    fprintf(2, "xargs: fork failed\n");
    return;
  }
  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "xargs: exec %s failed\n", args[0]);
    exit(1);
  }
  wait(0);
}

int
main(int argc, char *argv[])
{
  char *args[MAXARG];
  char line[512];
  int len = 0;          // characters in the current line
  int n;                // how many base arguments (the command itself)
  int i;
  int r;
  char c;

  if (argc < 2) {
    fprintf(2, "usage: xargs command [args...]\n");
    exit(1);
  }

  // args = the command from our own command line, e.g. {"echo", "bye"}
  n = 0;
  for (i = 1; i < argc; i++) {
    args[n] = argv[i];
    n++;
  }

  while (1) {
    r = read(0, &c, 1);           // read one character
    if (r <= 0) {
      break;                      // end of input
    }

    if (c == '\n') {
      line[len] = '\0';           // finish the line as a string
      args[n] = line;             // add the line as the last argument
      args[n + 1] = 0;            // exec needs the list to end with 0
      run(args);
      len = 0;                    // start a new line
    } else if (len < sizeof(line) - 1) {
      line[len] = c;
      len++;
    }
  }

  // the last line might not end with a newline
  if (len > 0) {
    line[len] = '\0';
    args[n] = line;
    args[n + 1] = 0;
    run(args);
  }

  exit(0);
}
/*
 * TASK: pipeline
 *
 * xv6's sh can connect commands with "|". In this task you do the same
 * thing yourself, using pipe(), fork(), exec(), close() and dup().
 *
 *   pipeline cmd1 [args...] : cmd2 [args...] [: cmd3 [args...] ...]
 *
 * The arguments are split into commands at every ":" (we can't use "|",
 * because sh would take it for itself). All the commands run AT THE SAME
 * TIME, and the standard output of each command is connected to the
 * standard input of the next one by a pipe. The first command reads
 * pipeline's own standard input, and the last one writes to pipeline's
 * own standard output. pipeline waits for every command to finish.
 *
 *   - If a command is empty (two ":" next to each other, or a ":" at the
 *     start or end), print "pipeline: empty command" and exit.
 *   - If exec fails, the child prints "pipeline: exec <cmd> failed".
 *
 * Remember: dup(fd) returns the LOWEST free file descriptor, so
 * close(1); dup(p[1]); makes fd 1 (standard output) the pipe's write end.
 * Every process must close every pipe end it doesn't use, or the reader
 * never sees end-of-file and the pipeline never finishes.
 *
 * Your solution should be in user/pipeline.c.
 * Hints: look at the PIPE case of runcmd() in user/sh.c.
 *        You don't need to copy the arguments: replace each ":" in argv
 *        with 0, and every command becomes a slice of argv that ends
 *        with 0, which is exactly what exec wants.
 *
 * Note: xv6's sh accepts at most 9 words on one command line
 * (MAXARGS in user/sh.c), otherwise it prints "too many args". The
 * ":"s count as words, so keep your test lines short.
 *
 * Sample commands and expected output:
 *   $ pipeline echo hello world : wc
 *   1 2 12
 *   $ pipeline cat README : grep xv6 : wc
 *   4 45 274
 *   $ pipeline echo a : xargs echo got : wc       (needs your xargs)
 *   1 2 6
 *   $ echo from stdin | pipeline cat : cat : cat
 *   from stdin
 *   $ pipeline echo hi : nosuchcmd
 *   pipeline: exec nosuchcmd failed
 *   $ pipeline echo hi :
 *   pipeline: empty command
 *   $ pipeline
 *   usage: pipeline cmd [args...] [: cmd [args...] ...]
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXCMD 10

int
main(int argc, char *argv[])
{
  char **cmd[MAXCMD];   // cmd[i] = the argv list of command i
  int ncmd = 0;
  int start = 1;        // index in argv where the current command starts
  int in = 0;           // fd the next command reads from (first: our stdin)
  int p[2];
  int pid;
  int i;

  if (argc < 2) {
    fprintf(2, "usage: pipeline cmd [args...] [: cmd [args...] ...]\n");
    exit(1);
  }

  // Split argv into commands. argv[argc] is already 0, so the last
  // command is ended for us.
  for (i = 1; i <= argc; i++) {
    if (i == argc || strcmp(argv[i], ":") == 0) {
      if (i == start) {
        fprintf(2, "pipeline: empty command\n");
        exit(1);
      }
      if (ncmd == MAXCMD) {
        fprintf(2, "pipeline: too many commands\n");
        exit(1);
      }
      argv[i] = 0;              // end this command's argument list
      cmd[ncmd] = &argv[start];
      ncmd++;
      start = i + 1;
    }
  }

  for (i = 0; i < ncmd; i++) {
    // every command except the last one writes into a new pipe
    if (i < ncmd - 1) {
      if (pipe(p) < 0) {
        fprintf(2, "pipeline: pipe failed\n");
        exit(1);
      }
    }

    pid = fork();
    if (pid < 0) {
      fprintf(2, "pipeline: fork failed\n");
      exit(1);
    }

    if (pid == 0) {
      // ---- child: set up fd 0 and fd 1, then become the command ----
      if (in != 0) {
        close(0);
        dup(in);                // fd 0 = read end of the previous pipe
        close(in);
      }
      if (i < ncmd - 1) {
        close(1);
        dup(p[1]);              // fd 1 = write end of the next pipe
        close(p[1]);
        close(p[0]);            // the NEXT command reads this, not us
      }
      exec(cmd[i][0], cmd[i]);
      fprintf(2, "pipeline: exec %s failed\n", cmd[i][0]);
      exit(1);
    }

    // ---- parent ----
    if (in != 0) {
      close(in);                // the child has its own copy now
    }
    if (i < ncmd - 1) {
      close(p[1]);              // only the child writes into this pipe
      in = p[0];                // the next command reads from it
    }
  }

  // all the commands are running: wait for every one of them
  for (i = 0; i < ncmd; i++) {
    wait(0);
  }
  exit(0);
}

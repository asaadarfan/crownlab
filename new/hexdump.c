/*
 * TASK: hexdump
 *
 * Write a program that prints the bytes of a file in hexadecimal,
 * 16 bytes per line. Each line starts with the offset (position in the
 * file) of its first byte as 8 hex digits, then a colon, then the bytes
 * as 2-digit hex numbers separated by spaces.
 *
 *   hexdump [file]
 *
 * If no file is given, read from standard input.
 * If the file cannot be opened, print "hexdump: cannot open <name>".
 *
 * xv6's printf has no "%02x" (no zero padding), so write a helper that
 * prints a number as an exact number of hex digits.
 *
 * Your solution should be in user/hexdump.c.
 *
 * Sample commands and expected output:
 *   $ echo hello | hexdump
 *   00000000: 68 65 6c 6c 6f 0a
 *   $ echo abcdefghijklmnopqrstuvwxyz | hexdump
 *   00000000: 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70
 *   00000010: 71 72 73 74 75 76 77 78 79 7a 0a
 *   $ hexdump nofile
 *   hexdump: cannot open nofile
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[512];
char digits[] = "0123456789abcdef";

// Print 'value' as exactly 'count' hex digits, e.g. (10, 2) -> "0a".
void
printhex(uint value, int count)
{
  int i;
  int d;

  for (i = count - 1; i >= 0; i--) {
    d = (value >> (i * 4)) & 0xf;   // pick out one hex digit (4 bits)
    printf("%c", digits[d]);
  }
}

void
hexdump(int fd)
{
  int n;
  int i;
  uint offset = 0;          // position in the file of the next byte

  while (1) {
    n = read(fd, buf, sizeof(buf));
    if (n <= 0) {
      break;
    }

    for (i = 0; i < n; i++) {
      if (offset % 16 == 0) {
        // start of a new line: print the offset
        printhex(offset, 8);
        printf(":");
      }

      printf(" ");
      printhex((uchar)buf[i], 2);   // uchar so bytes above 127 stay positive
      offset++;

      if (offset % 16 == 0) {
        printf("\n");               // 16 bytes done: end the line
      }
    }
  }

  if (offset % 16 != 0) {
    printf("\n");                   // finish a last, shorter line
  }
}

int
main(int argc, char *argv[])
{
  int fd;

  if (argc < 2) {
    hexdump(0);
    exit(0);
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "hexdump: cannot open %s\n", argv[1]);
    exit(1);
  }
  hexdump(fd);
  close(fd);
  exit(0);
}
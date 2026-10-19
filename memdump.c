#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
memdump(char *fmt, char *data, int len)
{
  int pos = 0;     // how many bytes of data we have used so far
  int i;
  char f;
  char *p;

  // One variable for each type we can print.
  int iv;          // for 'i' (4 bytes)
  uint64 pv;       // for 'p' (8 bytes)
  short hv;        // for 'h' (2 bytes)
  char cv;         // for 'c' (1 byte)
  char *sv;        // for 's' (8-byte address of a string)

  for (i = 0; fmt[i] != '\0'; i++) {
    f = fmt[i];        // the current format letter
    p = data + pos;    // where the unused data starts

    if (f == 'i') {
      if (len - pos < 4) {
        printf("memdump: not enough data for '%c'\n", f);
        return;
      }
      memmove(&iv, p, 4);
      printf("%d\n", iv);
      pos = pos + 4;

    } else if (f == 'p') {
      if (len - pos < 8) {
        printf("memdump: not enough data for '%c'\n", f);
        return;
      }
      memmove(&pv, p, 8);
      printf("%lx\n", pv);
      pos = pos + 8;

    } else if (f == 'h') {
      if (len - pos < 2) {
        printf("memdump: not enough data for '%c'\n", f);
        return;
      }
      memmove(&hv, p, 2);
      printf("%d\n", hv);
      pos = pos + 2;

    } else if (f == 'c') {
      if (len - pos < 1) {
        printf("memdump: not enough data for '%c'\n", f);
        return;
      }
      memmove(&cv, p, 1);
      printf("%c\n", cv);
      pos = pos + 1;

    } else if (f == 's') {
      if (len - pos < 8) {
        printf("memdump: not enough data for '%c'\n", f);
        return;
      }
      memmove(&sv, p, 8);
      printf("%s\n", sv);
      pos = pos + 8;

    } else if (f == 'S') {
      while (pos < len && data[pos] != '\0') {
        printf("%c", data[pos]);
        pos = pos + 1;
      }
      printf("\n");
      pos = len;    // S uses up the rest of the data
    }
  }
}

struct example {
  char *name;      // 8 bytes: a pointer to a string
  int num;         // 4 bytes
  short small;     // 2 bytes
  char letter;     // 1 byte
};

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    // No arguments: run some examples.

    printf("Example 1:\n");
    int nums[2];
    nums[0] = 61810;
    nums[1] = 2026;
    memdump("ii", (char *)nums, sizeof(nums));

    printf("Example 2:\n");
    char *str = "a string";
    memdump("s", (char *)&str, sizeof(str));

    printf("Example 3:\n");
    char text[] = "another";
    memdump("S", text, sizeof(text));

    printf("Example 4:\n");
    struct example ex;
    ex.name = "xyzzy";
    ex.num = 2026;
    ex.small = 100;
    ex.letter = 'z';
    memdump("sihc", (char *)&ex, sizeof(ex));

    exit(0);
  }

  // One argument: read all of standard input, then dump it.
  char buf[1024];
  int n = 0;    // total bytes read so far
  int r;        // bytes read by one call

  while (1) {
    r = read(0, buf + n, sizeof(buf) - n);
    if (r <= 0) {
      break;
    }
    n = n + r;
  }

  memdump(argv[1], buf, n);
  exit(0);
}
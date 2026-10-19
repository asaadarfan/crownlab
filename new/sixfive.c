#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"
char buf[512];


int is_sep(char c)
{
    char *seps = " -\r\t\n./,";
    for(int i = 0 ; seps[i]!= '\0' ; i++)
    {
        if (c == seps[i])
        {
            return 1;
        }
    }
    return 0;
}


void check(int num, int len, int valid)
{
    if(len == 0)
    {
        return;
    }

    if(valid == 0)
    {
        return;
    }

    if(num % 5 == 0 || num % 6 == 0)
    {
        printf("%d\n" , num);
    }
}


void sixfive(int fd)
{
    int n;
    int i;
    char c;
    int num = 0;
    int len = 0;
    int valid =1;

    while(1)
    {
        n = read(fd, buf, sizeof(buf));
        if(n<=0)
        {
            break;
        }

        for(i=0; i<n; i++)
        {
            c = buf[i];

            if(is_sep(c))
            {
                check(num, len, valid);
                num = 0;
                len = 0;
                valid = 1;
            }
            else 
            {
                if(c>='0' && c<='9')
                {
                    num = num*10 + (c-'0');
                }else
                {
                    valid = 0;
                }
                len++;
            }
        }
    }
    check(num, len, valid);
}



int main(int argc, char *argv[])
{
    int fd;
    int i;

    if(argc<2)
    {
        sixfive(0);
        exit(0);
    }

    for(i = 1 ; i<argc; i++)
    {
        fd = open(argv[i], O_RDONLY);
        if(fd<0)
        {
            fprintf(2, "sixfive: cannot open %s\n", argv[i]);
            continue;
        }
        sixfive(fd);
        close(fd);

    }
    exit(0);
}
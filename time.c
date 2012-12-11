#include <stdio.h>
#include <time.h>

int main()
{
    time_t stime;
    printf("%lu",time(&stime));
    printf("%s",asctime(gmtime(&stime)));
}

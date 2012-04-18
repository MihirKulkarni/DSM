#include <stdio.h>
#include <time.h>

int main()
{
    time_t timer;
    char buffer[25];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 25, "%m-%d-%Y %H:%M:%S", tm_info);
    puts(buffer);

    return 0;
}


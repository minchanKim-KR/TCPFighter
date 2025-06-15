#include "WriteLogTXT.h"

void WriteLog(const char* msg, const char* state, const char* msg2)
{
    time_t timer = time(NULL);
    tm t;
    localtime_s(&t, &timer);


    FILE* f;
    fopen_s(&f, "Log.txt", "at");

    // CONSOLE LOG
    printf("////////////////////////////////////////////////\n");
    printf("%d / %d / %d / %d:%d:%d \n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    printf("MSG : %s\n", msg);
    if (state != nullptr)
    {
        printf("TARGET : %s\n", state);
    }
    if (msg2 != nullptr)
    {
        printf("MSG2 : %s\n", msg2);
    }

    // FILE LOG
    if (f != nullptr)
    {
        fputs("////////////////////////////////////////////////\n", f);
        fprintf_s(f, "%d / %d / %d / %d:%d:%d \n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        fprintf_s(f, "MSG : %s\n", msg);

        if (state != nullptr)
        {
            fprintf_s(f, "TARGET : %s\n", state);
        }
        if (msg2 != nullptr)
        {
            fprintf_s(f, "MSG2 : %s\n", msg2);
        }

        fclose(f);
    }
}
#pragma once

#include <cstdio>

#ifdef _WIN32

inline FILE* blaze_popen(
    const char* command,
    const char* mode
)
{
    return _popen(command, mode);
}

inline int blaze_pclose(FILE* pipe)
{
    return _pclose(pipe);
}

#else

#include <sys/wait.h>

inline FILE* blaze_popen(
    const char* command,
    const char* mode
)
{
    return popen(command, mode);
}

inline int blaze_pclose(FILE* pipe)
{
    const int status = pclose(pipe);

    if (status == -1)
    {
        return -1;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return -1;
}

#endif
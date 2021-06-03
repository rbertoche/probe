#include <time.h>

// Tempo real fÃ¡cil
// double serve pra maioria dos casos
double rt_clock(void)
{
#ifdef _POSIX_C_SOURCE
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + (t.tv_nsec / 1E9);
#else //_POSIX_C_SOURCE
#if __STDC_VERSION__ >= 201112L
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return t.tv_sec + (t.tv_nsec / 1E9);
#else // __STDC_VERSION__

#pragma message "This module currently requires C11 \
    or POSIX extensions. \
    Activate it or port it here to your time.h \
    rt_clock() will return 0.0 for now \n\
    Esse mÃ³dulo atualmetne requer C11 ou as " \
    "extensÃµes POSIX. \
    Ative-o ou faÃ§a um port aqui para o seu time.h \
    rt_clock() retornarÃ¡ 0.0 por enquanto"
    return 0.0;
#endif // __STDC_VERSION__
#endif //_POSIX_C_SOURCE
}


#include <Windows.h>

#define __try                                                        \
{                                                                    \
    __label__ l_start, l_end, l_handler, l_setup_handler, l_seh_end; \
    goto l_setup_handler;                                            \
    l_start:

// TODO: implement for x86 32 bit

#if defined(__x86_64)

#define __except(filter)                                                  \
    goto l_seh_end;                                                       \
    l_handler:                                                            \
    {                                                                     \
        EXCEPTION_POINTERS *ep;                                           \
        __asm__ __volatile__(                                             \
            "movq %%rcx, %0\n\t"                                          \
            "push %%rbp\n\t"                                              \
            "push %%rdi" : "=r"(ep) :: "%rcx");                           \
        auto er = ep->ExceptionRecord;                                    \
        auto _exception_code = [er] { return er->ExceptionCode; };        \
        auto _exception_info = [er] { return er->ExceptionInformation; }; \
        long result = (filter);                                           \
        __asm__ __volatile__(                                             \
            "mov %[result], %%eax\n\t"                                    \
            "pop %%rdi\n\t"                                               \
            "pop %%rbp\n\t"                                               \
            "ret" :: [result] "r"(result) : "%eax");                      \
    }                                                                     \
    l_setup_handler:                                                      \
    __asm__ __volatile__ goto(                                            \
        ".seh_handler __C_specific_handler, @except\n\t"                  \
        ".seh_handlerdata\n\t"                                            \
        ".long 1\n\t"                                                     \
        ".rva %l[l_start], %l[l_end], %l[l_handler], %l[l_end]\n\t"       \
        ".text" :::: l_start, l_end, l_handler);                          \
    goto l_start;                                                         \
    l_end:

#else

#define __except(filter) \
    goto l_seh_end;      \
    l_handler:           \
    l_setup_handler:     \
    goto l_start;        \
    l_end:

#endif

#define __except_end l_seh_end: ; \
}

#define __leave goto l_seh_end

#define _try __try
#define _except __except

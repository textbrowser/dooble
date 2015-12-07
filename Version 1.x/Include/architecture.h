#if defined(__alpha__)
#define ARCHITECTURE_STR "__alpha__"
#elif defined(__alpha_ev4__)
#define ARCHITECTURE_STR "__alpha_ev4__"
#elif defined(__alpha_ev5__)
#define ARCHITECTURE_STR "__alpha_ev5__"
#elif defined(__alpha_ev6__)
#define ARCHITECTURE_STR "__alpha_ev6__"
#elif defined(__amd64__)
#define ARCHITECTURE_STR "__amd64__"
#elif defined(__amd64)
#define ARCHITECTURE_STR "__amd64"
#elif defined(__x86_64__)
#define ARCHITECTURE_STR "__x86_64__"
#elif defined(__x86_64)
#define ARCHITECTURE_STR "__x86_64"
#elif defined(__ARM_ARCH_6__)
#define ARCHITECTURE_STR "__ARM_ARCH_6__"
#elif defined(__ARM_ARCH_6J__)
#define ARCHITECTURE_STR "__ARM_ARCH_6J__"
#elif defined(__ARM_ARCH_6K__)
#define ARCHITECTURE_STR "__ARM_ARCH_6K__"
#elif defined(__ARM_ARCH_6Z__)
#define ARCHITECTURE_STR "__ARM_ARCH_6Z__"
#elif defined(__ARM_ARCH_6ZK__)
#define ARCHITECTURE_STR "__ARM_ARCH_6ZK__"
#elif defined(__ARM_ARCH_6T2__)
#define ARCHITECTURE_STR "__ARM_ARCH_6T2__"
#elif defined(__ARM_ARCH_7__)
#define ARCHITECTURE_STR "__ARM_ARCH_7__"
#elif defined(__ARM_ARCH_7A__)
#define ARCHITECTURE_STR "__ARM_ARCH_7A__"
#elif defined(__ARM_ARCH_7R__)
#define ARCHITECTURE_STR "__ARM_ARCH_7R__"
#elif defined(__ARM_ARCH_7M__)
#define ARCHITECTURE_STR "__ARM_ARCH_7M__"
#elif defined(__ARM_ARCH_7S__)
#define ARCHITECTURE_STR "__ARM_ARCH_7S__"
#elif defined(i386)
#define ARCHITECTURE_STR "i386"
#elif defined(__i386)
#define ARCHITECTURE_STR "__i386"
#elif defined(__i386__)
#define ARCHITECTURE_STR "__i386__"
#elif defined(__i486__)
#define ARCHITECTURE_STR "__i486__"
#elif defined(__i586__)
#define ARCHITECTURE_STR "__i586__"
#elif defined(__i686__)
#define ARCHITECTURE_STR "__i686__"
#elif defined(_X86_)
#define ARCHITECTURE_STR "_X86_"
#elif defined(__ia64__)
#define ARCHITECTURE_STR "__ia64__"
#elif defined(_IA64)
#define ARCHITECTURE_STR "_IA64"
#elif defined(__IA64__)
#define ARCHITECTURE_STR "__IA64__"
#elif defined(__powerpc)
#define ARCHITECTURE_STR "__powerpc"
#elif defined(__powerpc__)
#define ARCHITECTURE_STR "__powerpc__"
#elif defined(__powerpc64__)
#define ARCHITECTURE_STR "__powerpc64__"
#elif defined(__POWERPC__)
#define ARCHITECTURE_STR "__POWERPC__"
#elif defined(__ppc__)
#define ARCHITECTURE_STR "__ppc__"
#elif defined(__ppc64__)
#define ARCHITECTURE_STR "__ppc64__"
#elif defined(__PPC__)
#define ARCHITECTURE_STR "__PPC__"
#elif defined(__PPC64__)
#define ARCHITECTURE_STR "__PPC64__"
#elif defined(_ARCH_PPC)
#define ARCHITECTURE_STR "_ARCH_PPC"
#elif defined(_ARCH_PPC64)
#define ARCHITECTURE_STR "_ARCH_PPC64"
#elif defined(__sparc__)
#define ARCHITECTURE_STR "__sparc__"
#elif defined(__sparc)
#define ARCHITECTURE_STR "__sparc"
#elif defined(__sparc_v8__)
#define ARCHITECTURE_STR "__sparc_v8__"
#elif defined(__sparc_v9__)
#define ARCHITECTURE_STR "__sparc_v9__"
#elif defined(__sparcv8)
#define ARCHITECTURE_STR "__sparcv8"
#elif defined(__sparcv9)
#define ARCHITECTURE_STR "__sparcv9"
#else
#define ARCHITECTURE_STR "gingerjitter"
#endif

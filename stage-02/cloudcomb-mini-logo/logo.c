#include <stddef.h>
#include <syscall.h>
#include <signal.h>
#include <stdint.h>

#define SA_RESTORER 0x04000000

struct kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned long sa_flags;
	void (*sa_restorer) (void);
	sigset_t sa_mask;
};

uint8_t pic_meta[] = {89,
                      217,
                      169, a7,
                      166, 13,
                      150, 4, 9, 19, 9, 4,
                      149, 8, 2, 12, 3, 12, 2, 8,
                      150, 17, 11, 17,
                      151, 12, 19, 12,
                      147, 12, 27, 12,
                      143, 12, 35, 12,
                      140, 11, 43, 11,
                      139, 9, 49, 9,
                      138, 10, 49, 10,
                      138, 14, 41, 14,
                      138, 17, 35, 17,
                      138, 5, 5, 11, 27, 11, 5, 5,
                      138, 5, 8, 11, 21, 11, 8, 5,
                      138, 5, 12, 11, 13, 11, 12, 5,
                      138, 5, 15, 11, 7, 11, 15, 5,
                      138, 5, 19, 21, 19, 5,
                      138, 6, 21, 15, 21, 6,
                      138, 10, 21, 7, 21, 10,
                      138, 13, 43, 13,
                      138, 17, 35, 17,
                      138, 5, 4, 11, 29, 11, 4, 5,
                      138, 5, 8, 11, 21, 11, 8, 5,
                      138, 5, 11, 11, 15, 11, 11, 5,
                      138, 5, 15, 11, 7, 11, 15, 5,
                      138, 6, 17, 23, 18, 5,
                      138, 7, 20, 15, 21, 6,
                      139, 8, 21, 9, 21, 9,
                      140, 10, 45, 10,
                      143, 11, 37, 11,
                      146, 11, 31, 11,
                      150, 11, 23, 11,
                      153, 11, 17, 11,
                      157, 11, 9, 11,
                      160, 25,
                      164, 17,
                      167, 11,
                      170, 5,
                      171, 3,
                      172, 1,
                      129};

static int my_write(int fd, const void *buf, size_t size)
{
    long result;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "0"(__NR_write), "b"(fd), "c"(buf), "d"(size)
        : "cc", "memory");
    return result;
}

static inline int my_pause()
{
    long result;
    sigset_t mask = {};
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "0"(__NR_rt_sigsuspend), "b"(&mask), "c"(_NSIG / 8)
        : "cc", "memory");
    return result;
}

static void __restore_rt() {
    __asm__ __volatile__ ("mov    $0xf,%eax\n\t"
                          "syscall");
}

static inline void my_exit(int code)
{
    __asm__ __volatile__(
        "int $0x80"
        :
        : "a"(__NR_exit)
        : "cc", "memory");
    __builtin_unreachable(); // syscall above never returns
}

void sigint_handler(int ignored) {
    my_exit(0);
}

static inline int my_sigint() {
    long result;
    struct kernel_sigaction kact = {};

    kact.k_sa_handler = sigint_handler;
    kact.sa_flags = SA_RESTORER;
    kact.sa_restorer = __restore_rt;

    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "0"(__NR_rt_sigaction), "b"(SIGINT), "c"(&kact), "d"(0), "S"(_NSIG / 8)
        : "cc", "memory");

    return result;
}

void _start()
{
    char buf[8];
    int i, j, flip = 0;

    my_sigint();

    for (i = 0; i < sizeof(pic_meta); i++) {
        size_t len = pic_meta[i];
        if (len > 128) {
            len -= 128;
            my_write(1, "\n", 1);
            flip = 0;
	}
        for (j = 0; j < len; j++) {
            my_write(1, flip ? "." : " ", 1);
        }
        flip = !flip;
    }
    my_write(1, "\n", 1);

    while (1) {
        my_pause();
    }
}

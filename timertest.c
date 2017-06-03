#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

struct os_timer_cb
{
    void (* cb_func)(void *);
    void * cb_arg;
};

static void
cb1(int arg)
{
    printf("CALLBACK 1 %d\n", arg);
}

static void
cb2(int arg)
{
    printf("CALLBACK 2 %d\n", arg);
}

static void
print_siginfo(siginfo_t *si)
{
    // SIGNAL Calls siginfo
    struct os_timer_cb *otcb;
    otcb = (struct os_timer_cb*) si->si_value.sival_ptr;
    otcb->cb_func(otcb->cb_arg);
}

static void
handler(int sig, siginfo_t *si, void *uc)
{
    // SIGNAL information contains cb struct we had earlier. 
    struct os_timer_cb *otcb;
    
    // Pull the struct back from pointer
    otcb = (struct os_timer_cb*) si->si_value.sival_ptr;
    
    // Call the callback with callback arg
    otcb->cb_func(otcb->cb_arg);

    // Cancel/Ignore further signals
    //signal(sig, SIG_IGN);
}

int
main(int argc, char *argv[])
{
    timer_t timerid;
    timer_t timerid2;
    struct sigevent sev;
    struct itimerspec its;
    long long secs;
    sigset_t mask;
    struct sigaction sa;
    struct os_timer_cb ot_cb;
    struct os_timer_cb ot_cb2;

   if (argc != 3) {
        fprintf(stderr, "Usage: %s <sleep-secs> <freq-tenthsecs>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Establish handler for timer signal */

    printf("Establishing handler for signal %d\n", SIG);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIG, &sa, NULL) == -1)
        errExit("sigaction");

    /* Create the timer */
    // Use the first callback function with a value
    ot_cb.cb_func = cb1;
    ot_cb.cb_arg = 9;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &ot_cb;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");

    printf("timer ID 1 is 0x%lx\n", (long) timerid);

    /* Start the timer */

    secs = atoll(argv[2]);
    its.it_value.tv_sec = secs / 10;
    its.it_value.tv_nsec = 100 % 1000000000;
    
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1)
         errExit("timer_settime");

    // Use the second callback function for the second timer and a different value
    ot_cb2.cb_func = cb2;
    ot_cb2.cb_arg = 33;

    sev.sigev_value.sival_ptr = &ot_cb2;
    if (timer_create(CLOCKID, &sev, &timerid2) == -1)
        errExit("timer_create");

    printf("timer ID 2 is 0x%lx\n", (long) timerid2);

    /* Start the timer */

    secs = atoll(argv[2]);
    its.it_value.tv_sec = (secs * 2) / 10;
    its.it_value.tv_nsec = 100 % 1000000000;

    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid2, 0, &its, NULL) == -1)
         errExit("timer_settime");


   /* Sleep for a while; meanwhile, the timer may expire
       multiple times */

   struct itimerspec gits;
   struct itimerspec gits2;

   while(1) {
    printf("Sleeping for %d seconds\n", atoi(argv[1]));
    timer_gettime(timerid, &gits);
    timer_gettime(timerid2, &gits2);
    printf(" T1 Remaining Cur %d -- T2 Remaining %d \n", gits.it_value.tv_sec, gits2.it_value.tv_sec);
    sleep(atoi(argv[1]));
   }

   exit(EXIT_SUCCESS);
}
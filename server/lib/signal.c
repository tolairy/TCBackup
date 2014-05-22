#include "../global.h"

#ifndef _NSIG
    #define BA_NSIG 100
#else
    #define BA_NSIG _NSIG
#endif

static const char *sig_names[BA_NSIG+1];
typedef void (SIG_HANDLER)(int sig);
static SIG_HANDLER *exit_handler;


/*
 * Handle signals here
 */
void signal_handler(int sig)
{
    exit_handler(sig);
}

/*
 * Initialize signals
 */
void InitSignals(void terminate(int sig))
{
   struct sigaction sighandle;
#ifdef _sys_nsig
   int i;

   exit_handler = terminate;
   if (BA_NSIG < _sys_nsig)
      err_msg2(M_ABORT, 0, _("BA_NSIG too small (%d) should be (%d)\n"), BA_NSIG, _sys_nsig);

   for (i=0; i<_sys_nsig; i++)
      sig_names[i] = _sys_siglist[i];
#else
   exit_handler = terminate;
   sig_names[SIGINT]    = "Interrupt";
#endif

/* Now setup signal handlers */
   sighandle.sa_flags = 0;
   sighandle.sa_handler = signal_handler;
   sigfillset(&sighandle.sa_mask);

   sigaction(SIGINT,  &sighandle, NULL); //modified by hft 2011.6.27
   
}

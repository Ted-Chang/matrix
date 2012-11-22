#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#define SIGHUP		1	// Hangup
#define SIGINT		2	// Terminal interrupt signal
#define SIGQUIT		3	// Terminal quit signal
#define SIGILL		4	// Illegal instruction
#define SIGTRAP		5	// Trace trap
#define SIGABRT		6	// Process abort signal
#define SIGBUS		7	// Access to undefined portion of memory object
#define SIGFPE		8	// Erroneous arithmetic operation
#define SIGKILL		9	// Kill (cannot be caught or ignored)
#define SIGCHLD		10	// Child process terminated, stopped or continued
#define SIGSEGV		11	// Invalid memory reference
#define SIGSTOP		12	// Stop executing (cannot be caught or ignored)
#define SIGPIPE		13	// Write on a pipe with nobody to read it
#define SIGALRM		14	// Alarm clock
#define SIGTERM		15	// Termination signal
#define SIGUSR1		16	// User-defined signal 1
#define SIGUSR2		17	// User-defined signal 2
#define SIGCONT		18	// Continue execution, if stopped
#define SIGURG		19	// High bandwidth data is available at socket
#define SIGTSTP		20	// Terminal stop signal
#define SIGTTIN		21	// Background process attempting to read
#define SIGTTOU		22	// Background process attempting to write
#define SIGWINCH	23	// Window size change
#define NSIG		24	// Highest signal number

/* Signal bitmap type. Must be big enough to hold a bit for each signal */
typedef uint32_t sigset_t;

/* Type atomically accessible through asynchronous signal handlers */
typedef volatile int sig_atomic_t;

/* Signal stack information structure */
struct stack {
	void *ss_sp;		// Stack pointer
	size_t ss_size;		// Stack size
	int ss_flags;		// Flags
};
typedef struct stack stack_t;

/* Signal information structure passed to a signal handler */
struct siginfo {
	int si_signo;		// Signal number
	int si_code;		// Signal code
	int si_errno;		// Error code associated with this signal
	void *si_addr;		// Address of faulting instruction
	int si_status;		// Exit value or signal
};
typedef struct siginfo siginfo_t;

struct thread;

#endif	/* __SIGNAL_H__ */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "proc/signal.h"
#include "proc/thread.h"
#include "proc/process.h"

#define SIG_DFT_IGNORE(s) \
	((s) == SIGCHLD || (s) == SIGURG || (s) == SIGWINCH)

#define SIG_DFT_STOP(s) \
	((s) == SIGSTOP || (s) == SIGTSTP || (s) == SIGTTIN || (s) == SIGTTOU)

#define SIG_DFT_CORE(s) \
	((s) == SIGQUIT || (s) == SIGILL || (s) == SIGTRAP || (s) == SIGABRT || \
	 (s) == SIGBUS || (s) == SIGFPE || (s) == SIGSEGV)

#define SIG_DFT_TERM(s) \
	((s) == SIGHUP || (s) == SIGINT || (s) == SIGKILL || (s) == SIGPIPE || \
	 (s) == SIGALRM || (s) == SIGTERM || (s) == SIGUSR1 || (s) == SIGUSR2)

#define SIG_DFT_CONT(s)	((s) == SIGCONT)

void signal_send(struct thread *t, int num, struct siginfo *info, boolean_t force);

static void signal_force(struct thread *t, int num, int cause)
{
	/* If failed to deliver the signal that we're sending now, force it to run
	 * with the default action. This will usually cause the process being killed.
	 */
	if (num == cause) {
		t->owner->signal_act[num].sa_handler = SIG_DFT;
	}

	signal_send(t, num, NULL, TRUE);
}

void signal_send(struct thread *t, int num, struct siginfo *info, boolean_t force)
{
	if (info) {
		info->si_signo = num;
	}

	/* If we need to force and the signal is ignored or masked, override this
	 * and set to default action or unblock. POSIX allows us to do this.
	 */
	if (force) {
		t->signal_mask &= ~(1 << num);
		t->owner->signal_mask &= ~(1 << num);
		if (t->owner->signal_act[num].sa_handler == SIG_IGN) {
			t->owner->signal_act[num].sa_handler = SIG_DFT;
		}
	}

	/* Store information on the signal and mark it as pending */
	t->pending_signals |= (1 << num);
	if (info) {
		memcpy(&t->signal_info[num], info, sizeof(struct siginfo));
	} else {
		memset(&t->signal_info[num], 0, sizeof(struct siginfo));
		t->signal_info[num].si_signo = num;
	}

	/* Interrupt the thread if it is currently in interruptible sleep */
	thread_interrupt(t);
}

void signal_handle_pending()
{
	int i;
	sigset_t pending, mask;
	struct sigaction *action;

	pending = CURR_THREAD->pending_signals;
	pending &= ~(CURR_THREAD->signal_mask | CURR_PROC->signal_mask);
	if (!pending) {
		return;
	}

	for (i = 0; i < NSIG; i++) {
		if (!(pending & (1 << i))) {
			continue;
		}

		CURR_THREAD->pending_signals &= ~(1 << i);

		/* Check if the signal is ignored */
		action = &CURR_PROC->signal_act[i];
		if (action->sa_handler == SIG_IGN ||
		    (action->sa_handler == SIG_DFT && SIG_DFT_IGNORE(i))) {
			continue;
		}

		if (action->sa_handler != SIG_DFT) {
			;
		}

		/* Handle the default action */
		if (SIG_DFT_TERM(i)) {
			;
		} else if (SIG_DFT_CORE(i)) {
			;
		} else if (SIG_DFT_STOP(i)) {
			;
		} else if (SIG_DFT_CONT(i)) {
			break;
		}
	}
}

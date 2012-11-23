#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "proc/signal.h"
#include "proc/thread.h"
#include "proc/process.h"

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
	}
}

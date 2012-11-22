#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "proc/signal.h"

static void signal_force(struct thread *t, int num, int cause)
{
	/* If failed to deliver the signal that we're sending now, force it to run
	 * with the default action. This will usually cause the process being killed.
	 */
	if (num == cause) {
		;
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
		;
	}

	/* Store information on the signal and mark it as pending */
	if (info) {
		;
	} else {
		;
	}

	/* Interrupt the thread if it is currently in interruptible sleep */
	thread_interrupt(t);
}

void signal_handle_pending()
{
	;
}

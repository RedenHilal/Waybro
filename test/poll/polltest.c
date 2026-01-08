#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#include "poll.h"

#define TIMEOUT_MS 1000

static void test_multi_registration_reap(void)
{
	int fds[2];
	struct wb_poll_event ev;

	assert(pipe(fds) == 0);

	struct wb_poll_fort *fort =
		wb_poll_create(O_CLOEXEC, 0);
	assert(fort);

	void *d1 = (void *)0xAAA1;
	void *d2 = (void *)0xAAA2;
	void *d3 = (void *)0xAAA3;

	wb_poll_reg_events(fort, fds[0], WB_EVENT_READ, d1);
	wb_poll_reg_events(fort, fds[0], WB_EVENT_READ, d2);
	wb_poll_reg_events(fort, fds[0], WB_EVENT_READ, d3);

	write(fds[1], "X", 1);

	int n = wb_poll_wait_events(fort, &ev, 1, TIMEOUT_MS);
	assert(n == 1);

	assert(ev.fd == fds[0]);
	assert(ev.ev_mask & WB_EVENT_READ);

	void *r1 = wb_poll_reap_event(fort, &ev);
	void *r2 = wb_poll_reap_event(fort, &ev);
	void *r3 = wb_poll_reap_event(fort, &ev);
	void *r4 = wb_poll_reap_event(fort, &ev);

	assert(ev.count == 3);
	assert(r1 != NULL);
	assert(r2);
	assert(r3);
	
	assert(r1 && r2 && r3);
	assert(r4 == NULL);

	assert(r1 != r2);
	assert(r1 != r3);
	assert(r2 != r3);

	printf("[OK] multi-registration reap\n");

	close(fds[0]);
	close(fds[1]);
}

static void test_remove_one_registration(void)
{
	int fds[2];
	struct wb_poll_event ev;

	assert(pipe(fds) == 0);

	struct wb_poll_fort *fort =
		wb_poll_create(O_CLOEXEC, 0);
	assert(fort);

	void *d1 = (void *)0xBEEF;
	void *d2 = (void *)0xCAFE;

	struct wb_poll_handle *h1 =
		wb_poll_reg_events(fort, fds[0], WB_EVENT_READ, d1);
	wb_poll_reg_events(fort, fds[0], WB_EVENT_READ, d2);

	wb_poll_rmv_events(h1);

	write(fds[1], "Y", 1);

	int n = wb_poll_wait_events(fort, &ev, 1, TIMEOUT_MS);
	assert(n == 1);
	assert(ev.fd == fds[0]);

	void *r1 = wb_poll_reap_event(fort, &ev);
	void *r2 = wb_poll_reap_event(fort, &ev);

	assert(r1 == d2);
	assert(r2 == NULL);

	printf("[OK] remove single registration\n");

	close(fds[0]);
	close(fds[1]);
}

static void test_remove_all_registrations(void)
{
	int fds[2];
	struct wb_poll_event ev;

	assert(pipe(fds) == 0);

	struct wb_poll_fort *fort =
		wb_poll_create(O_CLOEXEC, 0);
	assert(fort);

	struct wb_poll_handle *h =
		wb_poll_reg_events(fort, fds[0], WB_EVENT_READ, (void *)0x1234);

	wb_poll_rmv_events(h);

	write(fds[1], "Z", 1);

	int n = wb_poll_wait_events(fort, &ev, 1, 100);
	assert(n == 0);

	printf("[OK] no event after all removals\n");

	close(fds[0]);
	close(fds[1]);
}

int main(void)
{
	test_multi_registration_reap();
	test_remove_one_registration();
	test_remove_all_registrations();

	printf("\nAll wb_poll public API tests passed \n");
	return 0;
}


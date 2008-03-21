//===----------------- main.cc - Mvm common threads -----------------------===//
//
//                              Mvm
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ctthread.h"
#include <stdio.h>
#include <sys/time.h>

LockReccursive	* l;
Cond						* c;
int go=0;

extern "C" int getchar(void);

int f(void *arg) {
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	l->lock();
	while(!go) {
		printf("Wait %d\n", Thread::self());
		c->timed_wait(l, &tv);
	}
	l->unlock();
	printf("Salut\n");
	return 0;
}

void pre(void *a) {
	printf("Pré %p\n", a);
}

void post(void *a) {
	printf("Post %p\n", a);
}

class Zop {
public:
	static Key<Zop>		* k;
	static void initialise_Key(void *pk) {
		printf("Init %p %p\n", pk, &k);
		((Key<Zop>*)pk)->set(new Zop());
		printf("GOOD\n");
	}

	static void duplicate_for_thread(void *, void *z) {
		printf("Dup %p\n", z);
		k->set(new Zop());
	}

	static void destroy_Key(void *z) {
		printf("************ Del %p\n", z);
		delete (Zop *)z;
	}
};

Key<Zop>*	Zop::k;

int main(int argc, char **argv) {
	printf("START\n");
  ctthread_t tid;
  
  printf("Initialise thread...\n");
	Thread::initialise();
  printf("Create Key...\n");
  
  Zop::k = new Key<Zop>();
  l = new LockReccursive();
  c = new Cond();
  printf("OK\n");

	Thread::register_handler(pre, post, (void *)0x1000);
	Thread::remove_handler(Thread::register_handler(pre, post, (void *)0x1000));

	Thread::start(&tid, f, (void *)22);
	Thread::start(&tid, f, (void *)33);

	getchar();
	l->lock();
	go = 1;
	c->broadcast();
	l->unlock();

	printf("On est passé\n");

	getchar();
	Thread::exit(22);
}


#include "msg.h"
#include <stdlib.h>

size_t msg_sizeof(int layers)
{
	size_t total = 0;

	switch (layers) { /* using fall-through. */
	case 3: total += sizeof(l3msg_t);
	case 2: total += sizeof(l2msg_t);
	default:
		return total;
	}
}

l2msg_t *msg_get_l2(msg_t *m)
{
	return (struct l2msg_t*) m->pl;
}

l3msg_t *msg_get_l3(msg_t *m)
{
	struct l2msg_t *l2 = msg_get_l2(m);
	return (struct l3msg_t*) l2->pl;
}

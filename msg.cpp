#include "msg.h"
#include <stdlib.h>

// size_t msg_sizeof(int layers)
// {
// 	size_t total = 0;
// 
// 	switch (layers) { /* using fall-through. */
// 	case 3: total += sizeof(l3msg_t);
// 	case 2: total += sizeof(l2msg_t);
// 	default:
// 		return total;
// 	}
// }

msg_t *msg_new(uint8_t len)
{
	msg_t *m = (msg_t*)malloc(sizeof(msg_t)+len);
	queue_entry_init((queue_entry*)m);
	m->len = len;
	return m;
}

void msg_free(msg_t *m)
{
	free(m);
}

#ifndef MSG_H
#define MSG_H
#include <stdint.h>
#include <stdlib.h>
#include "queue.h"

struct msg_t
{
	queue_entry entry;

	uint8_t dst;
	uint8_t len;
	uint8_t pl[];
};

enum
{
	MSG_L2_HELLO,
	MSG_L2_PING,
	MSG_L3_OGM,
	MSG_L3_ROGM,
	MSG_L3_KNOWN,
	MSG_PL_BROADCAST,
	MSG_PL_MULTICAST,
	MSG_PL_UNICAST
};

struct msg_header_t
{
	uint8_t l2_src; // 1
	uint8_t l3_src; // 2
	uint8_t l3_dst; // 3

	uint8_t seq:1;
	uint8_t type:7; // 4

	uint8_t pl[];
};

msg_t *msg_new(uint8_t len, bool raw = false);
msg_t *msg_dup(msg_t *m);
void   msg_free(msg_t *m);

msg_header_t *msg_get_header(msg_t *m);
uint8_t      *msg_get_pl    (msg_t *m);

#undef T
#endif /* MSG_H */

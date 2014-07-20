#ifndef MSG_H
#define MSG_H
#include <stdint.h>
#include <stdlib.h>
#include "queue.h"

struct msg_t{
	queue_entry entry;

	uint8_t dst;
	uint8_t len;
	uint8_t pl[];
};

#define MSG_HELLO
#define MSG_PING
#define MSG_OGM
#define MSG_ROGM
#define MSG_KNOWN
#define MSG_PL_BROADCAST
#define MSG_PL_MULTICAST
#define MSG_PL
struct msg_mesh_t {
	uint8_t l2_src; // 1
	uint8_t l3_src; // 2
	uint8_t l3_dst; // 3

	uint8_t seq:1;
	uint8_t type:7; // 4

	uint8_t pl[];
};

msg_t *msg_new(uint8_t len);
void   msg_free(msg_t *m);

#undef T
#endif /* MSG_H */

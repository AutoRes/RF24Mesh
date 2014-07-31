#ifndef MSG_H
#define MSG_H

#include "mesh_config.h"
#include "queue.h"

typedef struct
{
	queue_entry entry;

	addr_t dst;
	uint8_t len;
	uint8_t pl[];
} msg_t;

enum
{
	MSG_L2_HELLO,
	MSG_L2_PING,
	MSG_L3_OGM,
	MSG_L3_ROGM,
	MSG_L3_KNOWN,
	MSG_PL
};

typedef struct
{
	addr_t l2_src;
	addr_t l3_src;
	addr_t l3_dst;

	uint8_t seq:1;
	uint8_t type:7;

	uint8_t pl[];
} msg_header_t;

msg_t *msg_new(uint8_t len, bool raw);
msg_t *msg_dup(msg_t *m);
void   msg_free(msg_t *m);

msg_header_t *msg_get_header(msg_t *m);
uint8_t      *msg_get_pl    (msg_t *m);

#undef T
#endif /* MSG_H */

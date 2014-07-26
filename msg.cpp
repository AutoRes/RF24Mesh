#include "msg.h"
#include <stdlib.h>
#include <string.h>

msg_t *msg_new(uint8_t len, bool raw)
{
	uint8_t size = len;
	size += sizeof(msg_t);
	if(!raw) size += sizeof(msg_header_t);

	msg_t *m = (msg_t*)malloc(size);

	queue_entry_init((queue_entry*)m);
	m->len = size-sizeof(msg_t);

	return m;
}

msg_t *msg_dup(msg_t *m)
{
	msg_t *md = msg_new(m->len, true);

	msg_header_t *mh = msg_get_header(m);
	msg_header_t *mdh = msg_get_header(md);

	memcpy(mdh, mh, m->len);
	return md;
}

void msg_free(msg_t *m)
{
	free(m);
}

msg_header_t *msg_get_header(msg_t *m)
{
	return (msg_header_t*)m->pl;
}

uint8_t *msg_get_pl(msg_t *m)
{
	return msg_get_header(m)->pl;
}

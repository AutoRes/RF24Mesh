#ifndef MESH_H
#define MESH_H

#include "queue.h"
#include "msg.h"
#include "radio.h"
#include "layer2.h"
#include "layer3.h"

struct Mesh
{
	queue_head rx;
};

extern Mesh mesh;

void mesh_init(uint8_t self_addr);
void mesh_tick(void);

void mesh_send(msg_t *m, uint8_t to, uint8_t type);
void mesh_recv_irq(msg_t *m);
msg_t *mesh_recv(void);

#endif

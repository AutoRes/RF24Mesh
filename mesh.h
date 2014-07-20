#ifndef __MESH__
#define __MESH__

#include "radio.h"
#include "msg.h"

#define NB_MAX 32
#define NODES_MAX 127

typedef struct
{
	struct
	{
		uint8_t addr;
#define NODE_TIMEOUT 40
#define PONG_TIMER   1
		uint8_t timer;
	} nb[NB_MAX];
	uint8_t nb_l;

	struct
	{
		uint8_t hop:7;
		uint8_t seq:1;
	} nodes[NODES_MAX];
	uint8_t nodes_n;

#define HELLO_TIMER 4
	uint8_t hello_cnt;
#define OGM_TIMER 100
	uint8_t ogm_cnt;
} Mesh;

extern Mesh mesh;

void mesh_init(void);
void mesh_tick(void);

void mesh_l2_send(msg_t *m, uint8_t to);
void mesh_l3_send(msg_t *m, uint8_t to);

void mesh_l2_recv_irq(void);

void mesh_node_died(uint8_t addr);
void mesh_node_found(uint8_t addr);

#endif

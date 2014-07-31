#ifndef MESH_H
#define MESH_H

#include <stdlib.h>
#include "mesh_config.h"
#include "queue.h"
#include "msg.h"

typedef void (*irq_t)(void);

typedef struct
{
	irq_t app_irq;
	queue_head rx;
} Mesh;

extern Mesh mesh;

/*
 * Default pins map:
 * ----------------------------------
 * | Name | Arduino Pin | Radio Pin |
 * ----------------------------------
 * | GND  | GND         | 1         |
 * | 3V3  | 3V3         | 2         |
 * | CE   | 9           | 3         |
 * | CSN  | 10          | 4         |
 * | SCK  | 13          | 5         |
 * | MOSI | 11          | 6         |
 * | MISO | 12          | 7         |
 * | IRQ  | 2 (IRQ #0)  | 8         |
 * ----------------------------------
 */
void mesh_init(uint8_t self_addr,
		irq_t app_irq,
		uint8_t irq_n,
		uint8_t cepin,
		uint8_t cspin,
		uint32_t mesh_id);
void mesh_tick(void);

void mesh_send(msg_t *m, addr_t to);
void mesh_recv_irq(msg_t *m);
msg_t *mesh_recv(void);

#endif

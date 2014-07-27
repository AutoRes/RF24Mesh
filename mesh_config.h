#ifndef MESH_COMMON_H
#define MESH_COMMON_H

#include <stdint.h>
typedef uint8_t addr_t;

// radio

#define BCAST_ADDR 0xFF

#define BCAST_PIPE 2
#define SELF_PIPE  1

#define SEND_RETRY_DELAY   4 // 1 == 250us
#define SEND_NUM_RETRY_MAX 15

// layer2

typedef uint8_t nb_iter_t;

#define NUM_NB_MAX      32 // NB == NeighBour
#define NB_TIMER_MAX    20 // 1 == TICK_uS
#define HELLO_TIMER_MAX 4  // 1 == TICK_uS


// layer3

#define NUM_NODES_MAX 127
#define OGM_TIMER_MAX 100 // 1 == TICK_uS

// mesh

#define MESH_ID       0xC2C2C2C2

#define DEFAULT_IRQ_N 0
#define DEFAULT_CEPIN 9
#define DEFAULT_CSPIN 10

#define TICK_uS       50000


#endif

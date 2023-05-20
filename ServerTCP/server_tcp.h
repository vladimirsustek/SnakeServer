/*
 * server_tcp.h
 *
 *  Created on: Mar 10, 2022
 *      Author: 42077
 */

#ifndef SERVER_TCP_H_
#define SERVER_TCP_H_

#include <string.h>
#include <stdio.h>

#include "tcp.h"
#include "snake_port.h"

/*  protocol states */
enum tcp_server_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* structure for maintaing connection infos to be passed as argument
   to LwIP callbacks*/
struct tcp_server_struct
{
  u8_t state;             /* current connection state */
  struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
  struct pbuf *p;         /* pointer on the received/to be transmitted pbuf */
};

uint32_t* tcp_server_init(uint16_t port);

#endif /* SERVER_TCP_H_ */

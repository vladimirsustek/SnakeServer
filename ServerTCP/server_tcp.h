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


/* ECHO protocol states */
enum tcp_echoserver_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* structure for maintaing connection infos to be passed as argument
   to LwIP callbacks*/
struct tcp_echoserver_struct
{
  u8_t state;             /* current connection state */
  struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
  struct pbuf *p;         /* pointer on the received/to be transmitted pbuf */
};

void tcp_echoserver_init(void);

#endif /* SERVER_TCP_H_ */

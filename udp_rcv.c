/* -------------------------------------------------------------------------------------------------- */
/* The LongMynd receiver: udp_rcv.c                                                                       */
/*    - an implementation of the Serit NIM controlling software for the MiniTiouner Hardware          */
/*    - linux udp handler to send the TS data to a remote display                                     */
/* Copyright 2019 Heather Lomond                                                                      */
/* Copyright 2020 Andy Mace                                                                           */
/* -------------------------------------------------------------------------------------------------- */
/*
    This file is part of longmynd.

    Longmynd is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Longmynd is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with longmynd.  If not, see <https://www.gnu.org/licenses/>.
*/

/* -------------------------------------------------------------------------------------------------- */
/* ----------------- INCLUDES ----------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------- */

#include <errno.h>

#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h>
//#include <sys/utsname.h> 
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "errors.h"
#include "udp_rcv.h"
#include "main.h"

/* -------------------------------------------------------------------------------------------------- */
/* ----------------- GLOBALS ------------------------------------------------------------------------ */
/* -------------------------------------------------------------------------------------------------- */

struct sockaddr_in servaddr_rcv;
int sockfd_rcv;
u_char ttl;
struct ip_mreq mreq;

/* -------------------------------------------------------------------------------------------------- */
/* ----------------- DEFINES ------------------------------------------------------------------------ */
/* -------------------------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------------------------- */
/* ----------------- ROUTINES ----------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------- */

//UDPRecieve
void *loop_rcv(void *arg) {

  #define MAXLEN 1024
  thread_vars_t *thread_vars=(thread_vars_t *)arg;
  //  longmynd_status_t *status=(longmynd_status_t *)thread_vars->status;
  uint8_t *err = &thread_vars->thread_err;

  int n;
  int len;
  struct sockaddr_in from;
  char message [MAXLEN+1];
  //  struct utsname name;

  *err=ERROR_NONE;

  for (;;) {

    len=sizeof(from);
    if ( (n=recvfrom(sockfd_rcv, message, MAXLEN, 0, (struct sockaddr*)&from, &len)) < 0) {
      perror ("recv");
      exit(1);
    }

    ctrl_message_t *p = malloc(sizeof(ctrl_message_t));
    sscanf( message, "[GlobalMsg],Freq=%u,Offset=%u,Doppler=%u,Srate=%u,WideScan=%u,LowSR=%u,DVBmode=%s,FPlug=%s,Voltage=%c,22khz=%c", &p->frequency, &p->offset, &p->doppler, &p->SRate, &p->WideScan, &p->LowSR, &p->DVBmode, &p->FPlugA, &p->Voltage?"true":"false", &p->khz?"true":"false" );

    uint32_t freq = (p->frequency - p->offset);
    config_set_frequency_and_symbolrate(freq, p->SRate);
    config_set_lnbv(p->Voltage, p->khz);
    config_reinit(false);
    
    printf("Retune: %s", message);
  }


  return NULL;
}


/* -------------------------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------------------------- */
static uint8_t udp_r_init(struct sockaddr_in *servaddr_ptr, int *sockfd_ptr, char *udp_ip, int udp_port) {
  /* -------------------------------------------------------------------------------------------------- */
  /* initialises the udp socket                                                                         */
  /*    udp_ip: the ip address (as a string) of the socket to open                                      */
  /*  udp_port: the UDP port to be opened at the given IP address                                       */
  /*    return: error code                                                                              */
  /* -------------------------------------------------------------------------------------------------- */
  uint8_t err=ERROR_NONE;

  printf("Flow: UDP RCV Init\n");

  memset(servaddr_ptr, 0, sizeof(struct sockaddr_in));
  servaddr_ptr->sin_family = AF_INET;
  servaddr_ptr->sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr_ptr->sin_port = htons(udp_port);  

  if ( (*sockfd_ptr=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror ("recv socket");
    exit(1);
  }

  if (setsockopt(*sockfd_ptr, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
    perror("reuseaddr setsockopt");
     exit(1);
  }

  if (bind(*sockfd_ptr,(struct sockaddr*) servaddr_ptr, sizeof(*servaddr_ptr)) < 0) {
    perror ("bind");
    //    fprintf(stderr, "Socket bind error: %s\n", strerror(errno));
    exit(1);
  }
  
  //  / Preparatios for using Multicast
  mreq.imr_multiaddr.s_addr = inet_addr(udp_ip);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  // Tell the kernel we want to join that multicast group.
  if (setsockopt(*sockfd_ptr, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    perror ("add_membership setsockopt");
    exit(1);
  }
  
  if (err!=ERROR_NONE) printf("ERROR: UDP rcv init\n");

  return err;
}

uint8_t udp_rcv_init(char *udp_ip, int udp_port) {
  return udp_r_init(&servaddr_rcv, &sockfd_rcv, udp_ip, udp_port);
}



/* -------------------------------------------------------------------------------------------------- */
uint8_t udp_rcv_close(void) {
/* -------------------------------------------------------------------------------------------------- */
/* closes the udp socket                                                                              */
/* return: error code                                                                                 */
/* -------------------------------------------------------------------------------------------------- */
    uint8_t err=ERROR_NONE;
    int ret;

    printf("Flow: UDP RCV Close\n");

    ret=close(sockfd_rcv); 
    if (ret!=0) {
        err=ERROR_UDP_CLOSE;
        printf("ERROR: TS UDP close\n");
    }

    return err;
}

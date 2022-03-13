/*
 * Simple UDP client/server -- tests network stack using loopback address
 * 
 * Usage: 
 *    ./substation <-s>  --- run the server
 *    ./substation -- run the client and generate NMSGS messages
 */

#include <stdio.h>		/* printf */
#include <stdlib.h> 		/* EXIT_FAILURE & EXIT_SUCCESS */
#include <string.h>		/* memset */
#include <arpa/inet.h>		/* htons & inet_addr */
#include <sys/types.h>		/* socket calls */
#include <sys/socket.h>		/* socket calls */
#include <unistd.h>		/* close */

/* 
 * Experimental Server Network Properties
 * hostname: experimental
 * IP: 129.170.66.33
 * Port: 8880
 * Mask: 255.255.248.0
 * Gateway: 129.170.64.1
 * DNS: 129.170.17.4
 *
 */

#define SERV_ADDR "129.170.66.33" /* used by the client to connect */
//#define SERV_ADDR "127.0.0.1"	  /* used for testing */

#define UDP_ECHO_PORT 8880
#define TRUE 1
#define FALSE 0
#define errorExit(str) { printf(str); exit(EXIT_FAILURE); }

/* message types */
#define PING 1
#define UPDATE 2
#define CLASS_SIZE_MAX 30

typedef struct {
  int type;
  int id;
} Ping_t;

typedef struct {
  int type;
  int id;
  int value;
} Update_req_t;

typedef struct {
  int type;
  int id;
  int average;
  int values[CLASS_SIZE_MAX];
} Update_resp_t;

typedef union {
  Ping_t pingmsg;
  Update_req_t reqmsg;
  Update_resp_t respmsg;
} Msg_t;

/* some helper functions */
void print_msg(char *direction, Msg_t *msg,int msglen);
int build_reply(Msg_t *msg,Msg_t *reply, int *replylenp);
void get_input(int id,Msg_t *msg,int *len);

/* some useful macros */
#define type(msgp) (*((int*)msgp)) /* type is always first thing in every message */
#define id(msgp) (*((int*)(((uint8_t*)msgp)+sizeof(int)))) /* id second int in each message */
#define value(msgp) (((Update_req_t*)msgp)->value) /* only in req */
#define average(msgp)  (((Update_resp_t*)(msgp))->average) /* only in resp */
#define values(msgp,i) (((Update_resp_t*)(msgp))->values[i]) /* only in resp */

/* message buffers */
static Msg_t msgbuff;
static Msg_t replybuff;

/* the current class values */
static int classvalues[CLASS_SIZE_MAX];

int main(int argc, char **argv) {
  int server, sock, sent,recd, i;
  struct sockaddr_in echoaddr;
  unsigned int msglen,replylen, addrlen, response;
  int clientid;
	
  if(argc>=2 && (strcmp(argv[1],"-s")==0)) /* this is the server */
    server=TRUE;
  else if(argc==2 && (atoi(argv[1])>=0) && (atoi(argv[1])<CLASS_SIZE_MAX)) { /* legit client */
    clientid = atoi(argv[1]);
    server=FALSE;
  }
  else				/* invalid command line */
    errorExit("Usage: substation [-s] <id> -- 0<=id<30\n");
	
  /* clear the class values */
  for(i=0;i<CLASS_SIZE_MAX;i++) 	
    classvalues[i]=0;
  /* Create a UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    errorExit("Failed to create socket\n");
  /* Construct sockaddr_in structure */
  memset(&echoaddr, 0, sizeof(echoaddr));	/* Clear struct */
  echoaddr.sin_family = AF_INET;		/* Internet/IP */
  echoaddr.sin_port = htons(UDP_ECHO_PORT);	/* server port */
  addrlen = sizeof(echoaddr);
  /* client or server based on command line args */
  if(server) {		      
    printf("[Engs62 UDP Server]\n");
    echoaddr.sin_addr.s_addr = htonl(INADDR_ANY);   /* serve any IP address */
    if (bind(sock,(struct sockaddr *)&echoaddr,sizeof(echoaddr))<0) 
      errorExit("[SERVER] bind failure\n");
    while(1) { 			/* while not killed */
      memset(&msgbuff, 0, sizeof(Msg_t)); /* clear the buffers */
      memset(&replybuff, 0, sizeof(Msg_t)); 
      if ((recd=recvfrom(sock,(void*)&msgbuff,sizeof(Msg_t),0,
												 (struct sockaddr *)&echoaddr,&addrlen))<0)
				errorExit("[SERVER] recvfrom failure\n");
      print_msg("recv",&msgbuff,recd);
      build_reply(&msgbuff,&replybuff,&replylen);
      if(replylen>0) {		/* dont reply if message is bad */
				if ((sent=sendto(sock,(void*)&replybuff,replylen,0,
												 (struct sockaddr *)&echoaddr,sizeof(echoaddr))) != replylen)
					errorExit("[SERVER] sendto failure\n");
				print_msg("send",&replybuff,replylen);
      }
    }
  }  
  else {					     /* be a client */
    echoaddr.sin_addr.s_addr = inet_addr(SERV_ADDR); /* communicate with server IP */
    while(1) {					     /* while not terminated */
      memset(&msgbuff, 0, sizeof(Msg_t));                 /* clear the buffers */
      memset(&replybuff, 0, sizeof(Msg_t));                
      get_input(clientid,&msgbuff,&msglen);
      /* send a message */
      if ((sent=sendto(sock,(void*)&msgbuff,msglen,0, 
											 (struct sockaddr *)&echoaddr,sizeof(echoaddr))) != msglen) 
				errorExit("[CLIENT] sendto failure\n");
      print_msg("send",&msgbuff,sent);
      /* print the response */
      if ((recd=recvfrom(sock,(void*)&replybuff,sizeof(Msg_t),0,
												 (struct sockaddr *)&echoaddr,&addrlen))==0)
				errorExit("[CLIENT] recvfrom failure\n");
      print_msg("recv",&replybuff,recd);
    }
  }
  close(sock);
  return EXIT_SUCCESS;
}

/* server utilities */

int build_reply(Msg_t *msg,Msg_t *reply, int *replylenp) {
  int i;
  double sum;
  if(id(msg)>=0 && id(msg)<CLASS_SIZE_MAX) {
    type(reply) = type(msg);
    id(reply) = id(msg);
    switch(type(msg)) {
    case PING:			/* respond with a ping */
      *replylenp=sizeof(Ping_t);
			break;
    case UPDATE:		       /* respoond with an update */
      classvalues[id(msg)]=value(msg); /* update the value associate with id */
      for(sum=0,i=0;i<CLASS_SIZE_MAX; i++) { /* calculate the averate */
				sum+=classvalues[i];	
				values(reply,i)=classvalues[i]; /* put values in reply */
      }
      average(reply)=(int)(sum/(double)CLASS_SIZE_MAX); /* put average in reply */
      *replylenp=sizeof(Update_resp_t);
      break;
    default:
      *replylenp=0;		/* dont bother to reply */
      printf("Illegal Message type: ID=%d, type=%d\n",id(msg),type(msg));
      break;
    }
  }
  else {
    *replylenp=0;
    printf("Illegal ID=%d\n",id(msg));
  }
}

/* client utilities */

void get_input(int id,Msg_t *msg,int *len) {
  int intype,inval;
	
  printf("Send PING (1), UPDATE (2) ? : ");
  fflush(stdout);
  scanf("%d",&intype);
  if(intype==1) {
    type(msg)=PING;
    id(msg)=id;
    *len=sizeof(Ping_t);
  }
  else if(intype==2) {		/* ping */
    type(msg)=UPDATE;
    id(msg)=id;
    printf("Value ? : ");
    fflush(stdout);
    scanf("%d",&inval);
    value(msg)=inval;
    *len=sizeof(Update_req_t);
  }
  else {
    printf("exit\n");
    exit(EXIT_SUCCESS);
  }
}

/* 
 * print_msg -- prints any message, used by both client and server 
 */
void print_msg(char *direction, Msg_t *msg, int msglen) {
  int i;
  switch(type(msg)) {
  case PING:
    printf("%s: [PING,id=%d]\n",direction,id(msg));
    break;
  case UPDATE:
    if(msglen!=sizeof(Update_req_t) && msglen!=sizeof(Update_resp_t))
      printf("[Invalid UPDATE message,id=%d, length=%d]\n",id(msg),msglen);
    else {
      printf("%s: [UPDATE,id=%d",direction,id(msg));
      if(msglen==sizeof(Update_resp_t)) {
				printf(",average=%d,{",average(msg));
				for(i=0;i<CLASS_SIZE_MAX;i++)
					printf(" %d",values(msg,i));
				printf("}]");
      }
      else 
				printf(",value=%d]",value(msg));
      printf("\n");
    }
  }
}

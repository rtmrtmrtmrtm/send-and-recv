//
// c++ -O -std=c++11 -o tst tst.cc -lpthread
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <vector>
#include <thread>
#include "sarvec.h"

void
go(int s)
{
  struct sarargs args;
  struct sarvec sendvec;
  struct sarvec recvvec;
  char sendbuf[512];
  char recvbuf[512];

  args.sendvec = &sendvec;
  args.nsend = 0;
  args.recvvec = &recvvec;
  args.nrecv = 0;
  sendvec.fd = s;
  sendvec.name = 0;
  sendvec.namelen = 0;
  sendvec.data = sendbuf;
  sendvec.len = 0;
  recvvec.fd = s;
  recvvec.name = 0;
  recvvec.namelen = 0;
  recvvec.data = recvbuf;
  recvvec.len = 0;

  int sar = open("/dev/sar", 2);
  if(sar < 0){
    perror("/dev/sar");
    exit(1);
  }
  
  while(1){
    recvvec.len = sizeof(recvbuf);
    args.nrecv = 1;
    int err = write(sar, &args, sizeof(args));
    if(err < 0){
      perror("go() write");
      exit(1);
    }
    printf("tst recvvec.len %ld\n", recvvec.len);
  }
  close(s);
  close(sar);
}

int
main(int argc, char *argv[])
{
  int ret;
  int ns = 2;
  std::vector<std::thread*> tha;
  std::vector<int> wa;

  for(int i = 0; i < ns; i++){
    int ss[2];
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, ss);
    assert(ret == 0);
    std::thread *th = new std::thread(go, ss[0]);
    tha.push_back(th);
    wa.push_back(ss[1]);
  }

  int sar = open("/dev/sar", 2);
  if(sar < 0){
    perror("/dev/sar");
    exit(1);
  }

  struct sarvec vv[ns];
  char buf[ns];

  for(int i = 0; i < ns; i++){
    buf[i] = 'a' + i;
    struct sarvec *v = &vv[i];
    v->fd = wa[i];
    v->name = 0;
    v->namelen = 0;
    v->data = &buf[i];
    v->len = 1;
  }

  struct sarargs args;
  args.sendvec = vv;
  args.nsend = ns;
  args.recvvec = 0;
  args.nrecv = 0;
  if(write(sar, &args, sizeof(args)) <= 0){
    perror("write");
    exit(1);
  }

  sleep(1);
}

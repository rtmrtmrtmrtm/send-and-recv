//
// test send-and-receive
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

// read the descriptors in vector sv,
// and write data back to them.
void
go(std::vector<int> sv)
{
  int n = sv.size();
  struct sarargs args;
  struct sarvec sendvec[n];
  struct sarvec recvvec[n];

  args.sendvec = sendvec;
  args.nsend = n;
  args.recvvec = recvvec;
  args.nrecv = n;
  for(int i = 0; i < n; i++){
    sendvec[i].fd = sv[i];
    sendvec[i].name = 0;
    sendvec[i].namelen = 0;
    sendvec[i].data = (char*) malloc(512);
    sendvec[i].len = 0;
    recvvec[i].fd = sv[i];
    recvvec[i].name = 0;
    recvvec[i].namelen = 0;
    recvvec[i].data = (char*) malloc(512);
    recvvec[i].len = 0;
  }

  int sar = open("/dev/sar", 2);
  if(sar < 0){
    perror("/dev/sar");
    exit(1);
  }
  
  while(1){
    for(int i = 0; i < n; i++){
      recvvec[i].len = 512;
    }
    int err = write(sar, &args, sizeof(args));
    if(err < 0){
      perror("go() write");
      exit(1);
    }
    for(int i = 0; i < n; i++){
      printf("tst i=%d fd=%d recvvec.len %ld\n", i, sv[i], recvvec[i].len);
    }
  }
  for(int i = 0; i < n; i++){
    close(sv[i]);
  }
  close(sar);
}

int
main(int argc, char *argv[])
{
  int ret;
  int ns = 2;
  std::vector<int> wa;
  std::vector<int> ra;

  for(int i = 0; i < ns; i++){
    int ss[2];
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, ss);
    assert(ret == 0);
    wa.push_back(ss[1]);
    ra.push_back(ss[0]);
  }
  
  std::thread *th = new std::thread(go, ra);

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

  // send data on all of the descriptors.
  if(write(sar, &args, sizeof(args)) <= 0){
    perror("write");
    exit(1);
  }

  sleep(1);

  close(sar);
  for(int i = 0; i < ns; i++){
    close(wa[i]);
  }

}

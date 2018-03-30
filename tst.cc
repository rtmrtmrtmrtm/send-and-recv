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
  while(1){
    char buf[1024];
    int n = read(s, buf, sizeof(buf));
    printf("got %d from %d\n", n, s);
    if(n <= 0)
      break;
  }
  close(s);
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
  if(write(sar, &vv, sizeof(vv)) <= 0){
    perror("write");
    exit(1);
  }

  sleep(1);
}

//
// shared between user and kernel
//

struct sarargs {
  struct sarvec *sendvec;
  unsigned nsend;
  struct sarvec *recvvec;
  unsigned nrecv;
};

struct sarvec {
  int fd;
  void *name; // optional sockaddr
  unsigned int namelen;
  char *data;
  ssize_t len;
  int flags; // MSG_DONTWAIT
};

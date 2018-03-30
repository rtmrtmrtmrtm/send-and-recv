//
// shared between user and kernel
//

struct sarvec {
  int fd;
  void *name; // optional sockaddr
  unsigned int namelen;
  char *data;
  size_t len;
};

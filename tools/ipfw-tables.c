#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>

#define ERROR "1"

int main(int argc,char **argv)
{
  int i,j=0;

if (argc!=2) {
puts("Usage: ipfw-table <domain>");
return 1;
}

  if (res_init() != 0) {
    puts("error in init\n");
    return 1;
  }

  int anslen = 5000;
  unsigned char answer[anslen];

  int len = res_search(argv[1], C_IN, T_TXT, answer, anslen);

  if (len == -1) {
    puts(ERROR);
    return 1;
  }
  ns_msg msg;

  if (ns_initparse(answer, len, &msg) != 0) {
    puts(ERROR);
    return 1;
  }

  uint16_t msg_count = ns_msg_count(msg, ns_s_an);

  if (msg_count == 0)
    return 0;
  ns_rr rr;
  for (i = 0 ; i < msg_count; i++) {
    if (ns_parserr(&msg, ns_s_an, i, &rr))  {
    puts(ERROR);
      return 1;
    }
    char buf[MAXDNAME];
    int size = ns_name_uncompress(ns_msg_base(msg), ns_msg_end(msg), ns_rr_rdata(rr), buf, MAXDNAME);
    if (size < 0) {
    puts(ERROR);
      exit(1);
    }
    for (j = 0 ; j < size ; j++)
      if (buf[j] != 0x5c)
        printf("%c", buf[j]);
    printf("\n");
  }
  return 0;
}


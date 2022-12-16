#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
 
#define PORT_ubuntu "/var/lib/libvirt/qemu/vm-ubuntu.ctl"   //connect sock1
//#define PORT_fedora "/var/lib/libvirt/qemu/vm-fedora.ctl"   //connect sock2
 
int main(void) {
  
  int sock_fd1;
  struct sockaddr_un sock1;
  char buf_read[128];
  char buf_write[128];
  int ret;
  struct timeval time;
  int t=22;
 
  sock_fd1 = socket(AF_UNIX, SOCK_STREAM, 0);
  if ( sock_fd1 == -1 ) {
    fprintf(stderr, "Error: Socket1 can not be created !! \n");
          fprintf(stderr, "errno : %d\n", errno);
    return -1;
  }
  printf(" socket1 make success\n");
 
  sock1.sun_family = AF_UNIX;
  memcpy(&sock1.sun_path, PORT_ubuntu, sizeof(sock1.sun_path));
  ret = connect(sock_fd1, (struct sockaddr *)&sock1, sizeof(sock1));
  if ( ret == -1) {
    fprintf(stderr, "sock1 Connect Failed!\n");
    return -1;
  }
  printf(" connect success\n");
  
  while(t!=0){
    //第一次主钟发送消息，将他t1发送给从钟
    gettimeofday(&time,NULL);
    sprintf(buf_write, "%ld,%ld\n", time.tv_sec, time.tv_usec);
    ret = write(sock_fd1, buf_write, sizeof(buf_write));
    if(ret == -1)
    {
      fprintf(stderr,"can't write data from host\n");
      return -1;
    }
    printf("t1:%ld,%ld\n", time.tv_sec, time.tv_usec);
  
    //获取从钟第一次发送消息,此时为t4
    read(sock_fd1, buf_read, 128);
    gettimeofday(&time,NULL);
    sprintf(buf_write,"%ld,%ld\n",time.tv_sec,time.tv_usec);
    ret = write(sock_fd1, buf_write, sizeof(buf_write));
    if(ret == -1)
    {
      fprintf(stderr,"can't write data from host\n");
      return -1;
    }
    printf("t4:%ld,%ld\n", time.tv_sec, time.tv_usec);
  
    t--;
  }     
  close(sock_fd1);
  return 0;
}
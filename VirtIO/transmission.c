#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>

#define MAXFILE 65535
#define SERVER_PORT "/var/lib/libvirt/qemu/vm-ubuntu-1.ctl"
#define CLIENT_PORT "/var/lib/libvirt/qemu/vm-ubuntu.ctl"

int main()
{
    struct sockaddr_un sock_server;
    struct sockaddr_un sock_client;
    struct timespec time;    //纳秒
    int sock_fd_server;
    int sock_fd_client;
    //int fd_debug;   //调试文件
    int i,fd,ret,t;
    char buf_log[128];
    
    sock_fd_server = socket(AF_UNIX, SOCK_STREAM, 0);
    sock_fd_client = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( sock_fd_server == -1 || sock_fd_client == -1) {
        fprintf(stderr, "Error: Socket1 can not be created !! \n");
        fprintf(stderr, "errno : %d\n", errno);
        return -1;
    }
    printf(" sock_fd_server and sock_fd_client make success\n");
 
    sock_server.sun_family = AF_UNIX;
    memcpy(&sock_server.sun_path, SERVER_PORT, sizeof(sock_server.sun_path));
    ret = connect(sock_fd_server, (struct sockaddr *)&sock_server, sizeof(sock_server));
    if ( ret == -1) {
      fprintf(stderr, "sock_server Connect Failed!\n");
      return -1;
    }
    
    printf("sock_server connect success\n");
    
    sock_client.sun_family = AF_UNIX;
    memcpy(&sock_client.sun_path, CLIENT_PORT, sizeof(sock_client.sun_path));
    ret = connect(sock_fd_client, (struct sockaddr *)&sock_client, sizeof(sock_client));
    if ( ret == -1) {
      fprintf(stderr, "sock_client Connect Failed!\n");
      return -1;
    }
    
    printf("sock_client connect success\n");
    
    while(1){
    
    	ret = read(sock_fd_server,&time,sizeof(struct timespec));
        if(ret == -1)
        {
            fprintf(stderr,"can't read data from server\n");
            return -1;
        }
//        printf("server: %ld, %ld\n", time.tv_sec, time.tv_nsec);
        ret = write(sock_fd_client,&time,sizeof(struct timespec));
        if(ret == -1)
        {
            fprintf(stderr,"can't write data from client\n");
            return -1;
        }
        

        ret = read(sock_fd_client,&time,sizeof(struct timespec));
        if(ret == -1)
        {
            fprintf(stderr,"can't read data from server\n");
            return -1;
        }
//        printf("client: %ld, %ld\n", time.tv_sec, time.tv_nsec);
        ret = write(sock_fd_server,&time,sizeof(struct timespec));
        if(ret == -1)
        {
            fprintf(stderr,"can't write data from client\n");
            return -1;
        }
        

        ret = read(sock_fd_server,&time,sizeof(struct timespec));
        if(ret == -1)
        {
            fprintf(stderr,"can't read data from server\n");
            return -1;
        }
//        printf("server: %ld, %ld\n", time.tv_sec, time.tv_nsec);
        ret = write(sock_fd_client,&time,sizeof(struct timespec));
        if(ret == -1)
        {
            fprintf(stderr,"can't write data from client\n");
            return -1;
        }
        

    }
    
    close(sock_fd_server);
    close(sock_fd_client);
    
}

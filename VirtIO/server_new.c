#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>

#define MAXFILE 65535
#define PORT_ubuntu "/var/lib/libvirt/qemu/vm-ubuntu.ctl"   //connect sock1
#define Debug_LOG "/home/xiong/桌面/zsr/Inter-VM-PTP/VirtIO/debug.log"

int main()
{
    //变量定义需要整理下
    pid_t pc;
    struct sockaddr_un sock1;
    struct timeval time;
    int sock_fd1;
    int fd_debug;   //调试文件
    int i,fd,ret,t;
    char buf_read[128];
    char buf_write[128];
    char buf_log[128];
    int pid;
      
    pc =fork(); 
    if(pc<0)
    {
        printf("error fork \n");
        exit(1);
    } else if(pc>0) {
        exit(0);    // 结束父进程
    }
    setsid();       
    chdir("/");     
    umask(0);       
    for(i = 0;i < MAXFILE; i++)  
        close(i);
    pid = getpid();  //获取守护进程的pid
    while(1)
    {
        //打开记录测试过程的文件
        if((fd_debug=open(Debug_LOG,O_CREAT|O_WRONLY|O_APPEND,0600))<0)
        {
          printf("open file err\n");
          exit(0);
        }
        
      t=10;
      //记录10次测试时间
      sock_fd1 = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( sock_fd1 == -1 ) {
        fprintf(stderr, "Error: Socket1 can not be created !! \n");
          fprintf(stderr, "errno : %d\n", errno);
        return -1;
    }
    //printf(" socket1 make success\n");
 
    sock1.sun_family = AF_UNIX;
    memcpy(&sock1.sun_path, PORT_ubuntu, sizeof(sock1.sun_path));
    ret = connect(sock_fd1, (struct sockaddr *)&sock1, sizeof(sock1));
    if ( ret == -1) {
      fprintf(stderr, "sock1 Connect Failed!\n");
      return -1;
    }
    //printf(" connect success\n");
  
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
        //printf("t1:%ld,%ld\n", time.tv_sec, time.tv_usec);
        sprintf(buf_log,"t1:%s",buf_write);
          write(fd_debug,buf_log,strlen(buf_log));
        
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
        //printf("t4:%ld,%ld\n", time.tv_sec, time.tv_usec);
        sprintf(buf_log,"t4:%s\n",buf_write);
          write(fd_debug,buf_log,strlen(buf_log));
  
        t--;
    }     
    close(sock_fd1);
    close(fd_debug);
    sleep(9);
    usleep(800000);
    }
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/timex.h>

#define MAXFILE 65535
#define CONTROL_PORT "/dev/vport1p3"
#define OFFSET_LOG "/home/zsr/Desktop/connect/time/pid-time/offset.log"
#define Debug_LOG "/home/zsr/Desktop/connect/time/pid-time/debug.log"

void getTheTime( char* str, struct timeval *time);
long getOffset(struct timeval t1, struct timeval t2, struct timeval t3, struct timeval t4);
long getAveOffset(struct timeval off[][4],int n);
int setTheTime(long offSet);

int main()
{
      pid_t pc;
      struct timeval t1, t2, t3, t4;
      struct timeval time[11][4];
      int fd_con;    //通信文件
      int fd_log;    //记录文件
      int fd_debug;   //调试文件
      int pid;
      char buf_read[128];
      char buf_read_cp[128];
      char buf_write[128];
      char buf_log[128];
      int len,ret,t;
      long AveOffset;
      long offsettime[11];
    
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
        fd_con = open(CONTROL_PORT, O_RDWR);
        if(fd_con == -1){
          fprintf(stderr,"%d:can't open vport\n",errno);
          return -1;
        }
      
     while(t>=0){
          //获取主钟发来的第一次消息，记录t1,t2
          ret = read(fd_con,&time[t][0],sizeof(struct timeval));
          if(ret == -1){
              fprintf(stderr,"can't write data from guestOS\n");
              return -1;
          }
          
          gettimeofday(&time[t][1], NULL);

//          sprintf(buf_log,"buffer(t1):%ld,%ld\n",time[t][0].tv_sec,time[t][0].tv_usec);
//          write(fd_debug,buf_log,strlen(buf_log));

          //发送消息给主钟，记录t3
          gettimeofday(&time[t][2],NULL);
//          sprintf(buf_write,"%ld,%ld",t3.tv_sec,t3.tv_usec);
          write(fd_con,&time[t][2],sizeof(struct timeval));
          if(ret == -1){
              fprintf(stderr,"can't write data from guestOS\n");
              return -1;
          }

          //获取主钟消息，记录t4
          ret = read(fd_con,&time[t][3],sizeof(struct timeval));
          if(ret == -1){
              fprintf(stderr,"can't write data from guestOS\n");
              return -1;
          }

//          sprintf(buf_log,"buffer(t4):%ld,%ld\n",time[t][3].tv_sec,time[t][3].tv_usec);
//          write(fd_debug,buf_log,strlen(buf_log));
          //printf("t4:from host:%s", buf_read);
          
          //计算时间偏差
//          offsettime[10-t] = getOffset(t1, t2, t3, t4);
//          printf("offset: %ld us\n\n", offsettime[10-t]);
  
          t = t - 1;
      }
         //计算平均时间偏差
      AveOffset = getAveOffset(time,10);
      printf("offset: %ld us\n",AveOffset);
      //printf("Before, the average offset is %ld us\n", beforeOffset);
          
          //根据时间偏差修改系统时间
      ret = setTheTime(AveOffset);
      if(ret == -1){
          fprintf(stderr,"%d:set the time fail\n", errno);
          return -1;
      }
      //printf("修改参数为:%d\n\n", ret);
      sprintf(buf_log,"修改参数为:%d\n\n", ret);
          write(fd_debug,buf_log,strlen(buf_log));
        
        //将时间偏差记录在log文件里
          if((fd_log=open(OFFSET_LOG,O_CREAT|O_WRONLY|O_APPEND,0600))<0)
          {
                printf("open file err\n");
                exit(0);
          }
          sprintf(buf_log,"%d:the average offset is %ld us \n",pid,AveOffset);
          write(fd_log,buf_log,strlen(buf_log));
          close(fd_con);
          close(fd_debug);
          close(fd_log);
          sleep(10);
      }
}


//从读取到的消息中，截取时间信息
void getTheTime( char* str, struct timeval *time){
    int len;
    (*time).tv_sec = 0;
    (*time).tv_usec = 0;

    len = strlen(str);
    for(int i=0; i<len; i++)
    {
      for(;i<len;i++){
          if(str[i]==',')
              break;      
            (*time).tv_sec = (*time).tv_sec*10 +str[i]-'0';
        }
        if(str[i] == ',')
        {
            i++;
            for(; i<len; i++){
              if(str[i]=='\n')
                break;
                (*time).tv_usec = (*time).tv_usec*10 + str[i]-'0';
            }
        }
    }
}

//计算时间偏差
long getOffset(struct timeval t1, struct timeval t2, struct timeval t3, struct timeval t4){
  long offset = 0;
  offset = (t2.tv_sec*1000000+t2.tv_usec)-(t1.tv_sec*1000000+t1.tv_usec)-(t4.tv_sec*1000000+t4.tv_usec)+(t3.tv_sec*1000000+t3.tv_usec);
  offset = offset / 2;
  return offset;
}

//计算平均时间偏差
long getAveOffset(struct timeval off[][4],int n){
  long time_offset[n];
  long offset = 0;
  int i;
  
  for( i=0 ; i<n ; i++ ){
  	time_offset[i] = getOffset(off[i][0],off[i][1],off[i][2],off[i][3]);
  }
  
  for( i=0 ; i<n ; i++ ){
    offset = offset + time_offset[i];
  }
  offset = offset / 10;
  return offset;
}

//设置时间
int setTheTime(long offset){ 
  struct timeval time;
  int ret;
  time.tv_sec = 0;
  time.tv_usec = 0 - offset;
  ret = adjtime(&time, NULL);
  return ret;
}




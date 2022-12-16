#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define CONTROL_PORT "/dev/vport1p3"

void getTheTime( char* str, struct timeval *time);
long getOffset(struct timeval t1, struct timeval t2, struct timeval t3, struct timeval t4);
long getAveOffset(long off[],int n);
int setTheTime(long offSet);

int main(void){
  
  int fd;
  char buf_read[128];
  char buf_write[128];
  int len,ret;
  struct timeval t1, t2, t3, t4;
  long beforeOffset,afterOffset;
  int t=11;
  long offsettime[11];

  fd = open(CONTROL_PORT, O_RDWR);
  if(fd == -1){
    fprintf(stderr,"%d:can't open vport\n",errno);
    return -1;
  }
  
  //校正前计算时间偏差
  while(t!=0){
    //获取主钟发来的第一次消息，记录t1,t2
    ret = read(fd,buf_read,128);
    if(ret == -1){
      fprintf(stderr,"can't write data from guestOS\n");
      return -1;
    }
    gettimeofday(&t2, NULL);
    printf("t1:from host:%s", buf_read);
    getTheTime(buf_read, &t1);

    //发送消息给主钟，记录t3
    gettimeofday(&t3,NULL);
    sprintf(buf_write,"%ld,%ld",t3.tv_sec,t3.tv_usec);
    write(fd,buf_write,128);
    if(ret == -1){
      fprintf(stderr,"can't write data from guestOS\n");
      return -1;
    }

    //获取主钟消息，记录t4
    ret = read(fd,buf_read,128);
    if(ret == -1){
      fprintf(stderr,"can't write data from guestOS\n");
      return -1;
    }
    getTheTime(buf_read, &t4);
    printf("t4:from host:%s", buf_read);
  
    printf("t1:%ld,%ld\n", t1.tv_sec, t1.tv_usec);
    printf("t2:%ld,%ld\n", t2.tv_sec, t2.tv_usec);
    printf("t3:%ld,%ld\n", t3.tv_sec, t3.tv_usec);
    printf("t4:%ld,%ld\n", t4.tv_sec, t4.tv_usec);
  
    //计算时间偏差
    offsettime[t-1] = getOffset(t1, t2, t3, t4);
    printf("offset: %ld us\n\n", offsettime[t-1]);
  
    t--;
  }
  
  //计算平均时间偏差
  beforeOffset = getAveOffset(offsettime,10);
  printf("Before, the average offset is %ld us\n", beforeOffset);
  
  //根据时间偏差修改系统时间
  ret = setTheTime(beforeOffset);
  if(ret == -1){
    fprintf(stderr,"%d:set the time fail\n", errno);
    return -1;
  }
  printf("修改参数为:%d\n\n", ret);
  sleep(3);
  
  //校正后计算时间偏差
  t=11;
  while(t!=0){
    //获取主钟发来的第一次消息，记录t1,t2
    ret = read(fd,buf_read,128);
    if(ret == -1){
      fprintf(stderr,"can't write data from guestOS\n");
      return -1;
    }
    gettimeofday(&t2, NULL);
    printf("t1:from host:%s", buf_read);
    getTheTime(buf_read, &t1);

    //发送消息给主钟，记录t3
    gettimeofday(&t3,NULL);
    sprintf(buf_write,"%ld,%ld",t3.tv_sec,t3.tv_usec);
    write(fd,buf_write,128);
    if(ret == -1){
      fprintf(stderr,"can't write data from guestOS\n");
      return -1;
    }

    //获取主钟消息，记录t4
    ret = read(fd,buf_read,128);
    if(ret == -1){
      fprintf(stderr,"can't write data from guestOS\n");
      return -1;
    }
    getTheTime(buf_read, &t4);
    printf("t4:from host:%s", buf_read);
  
    printf("t1:%ld,%ld\n", t1.tv_sec, t1.tv_usec);
    printf("t2:%ld,%ld\n", t2.tv_sec, t2.tv_usec);
    printf("t3:%ld,%ld\n", t3.tv_sec, t3.tv_usec);
    printf("t4:%ld,%ld\n", t4.tv_sec, t4.tv_usec);
  
    //计算时间偏差
    offsettime[t-1] = getOffset(t1, t2, t3, t4);
    printf("offset: %ld us\n\n", offsettime[t-1]);
  
    t--;
  }
  
  //计算平均时间偏差
  afterOffset = getAveOffset(offsettime,10);
  printf("After, the average offset is %ld us\n", afterOffset);
  
  ret = close(fd);
  if(ret < 0){
    fprintf(stderr,"%d:can't close vport\n",ret);
    return -1;
  
  }
  
  return 0;
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
long getAveOffset(long off[],int n){
  long offset = 0;
  for(int i=0;i<n;i++){
    offset = offset + off[i];
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
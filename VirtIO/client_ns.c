#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/timex.h>

#define MAXFILE 65535
#define CONTROL_PORT "/dev/vport1p4"
#define OFFSET_LOG "/home/zsr/Desktop/Inter-VM-PTP/VirtIO/offset_ns.log"
#define Debug_LOG "/home/zsr/Desktop/Inter-VM-PTP/VirtIO/debug_ns.log"

void getTheTime( char* str, struct timespec *time);
long getOffset(struct timespec t1, struct timespec t2, struct timespec t3, struct timespec t4);
long getDelay(struct timespec t1, struct timespec t2, struct timespec t3, struct timespec t4);
long getAveOffset(struct timespec off[][4],int n);
long getAveDelay(struct timespec off[][4],int n);
int setTheTime(long offSet);
void write_info(struct timespec time[][4],int n,int fd_log);

int main()
{
      struct timeval time[2][4];
      struct timespec time_ns[2][4];
      int fd_con;    //通信文件
      int fd_log;    //记录文件
      int fd_debug;   //调试文件
      char buf_log[128];
      int ret ,set_ret, t;
      long AveOffset,AveDelay;
      long offsettime[2];
      int status = 0;        ////状态，0为波动状态，1为稳定状态
      int count = 0;
      int flag = 1;
      int exp_count = 0;
      

            //打开记录测试过程的文件
      if((fd_debug=open(Debug_LOG,O_CREAT|O_WRONLY|O_APPEND,0600))<0)
      {
            printf("open file err\n");
            exit(0);
      }
      
      fd_con = open(CONTROL_PORT, O_RDWR);
      if(fd_con == -1){
        fprintf(stderr,"%d:can't open vport\n",errno);
        return -1;
      }
      
      if((fd_log=open(OFFSET_LOG,O_CREAT|O_WRONLY|O_APPEND,0600))<0)
      {
            printf("open file err\n");
            exit(0);
      }
      
      while(1)
      {
      
        t=1;
      
     	 while(t>=0){
          //获取主钟发来的第一次消息，记录t1,t2
			   ret = read(fd_con,&time_ns[t][0],sizeof(struct timespec));
			   clock_gettime(CLOCK_REALTIME,&time_ns[t][1]);
          
           if(ret == -1){
              fprintf(stderr,"can't write data from guestOS\n");
              return -1;
           }

//          sprintf(buf_log,"buffer(t1):%ld,%ld\n",time_ns[t][0].tv_sec,time_ns[t][0].tv_nsec);
//          write(fd_debug,buf_log,strlen(buf_log));

          //发送消息给主钟，记录t3
			    clock_gettime(CLOCK_REALTIME,&time_ns[t][2]);
			    write(fd_con,&time_ns[t][2],sizeof(struct timespec));
            if(ret == -1){
              fprintf(stderr,"can't write data from guestOS\n");
              return -1;
            }

          //获取主钟消息，记录t4
			    ret = read(fd_con,&time_ns[t][3],sizeof(struct timespec));
            if(ret == -1){
              fprintf(stderr,"can't write data from guestOS\n");
              return -1;
            }
          
//          sprintf(buf_log,"buffer(t4):%ld,%ld\n",time_ns[t][3].tv_sec,time_ns[t][3].tv_nsec);
//          write(fd_debug,buf_log,strlen(buf_log));
  
            t--;
        }
         //计算平均时间偏差
        AveOffset = getAveOffset(time_ns,1);
        AveDelay = getAveDelay(time_ns,1);
          
        if( count == 10 || ( AveOffset >= -1000 && AveOffset <= 1000) ){
      	  flag = 1;
      	  count = 0;
      	  if(AveOffset >= -1000 && AveOffset <= 1000){
      	    //1为稳定状态
      	    status = 1;
      	  }else{
      	  	status = 0;
      	  }
        }
        //根据时间偏差修改系统时间
        if( flag == 1 ){
         	printf("offset: %ld ns, delay: %ld ns\n",AveOffset,AveDelay);
      	  set_ret = setTheTime(AveOffset);
      	  if(set_ret == -1)
      	  {
            fprintf(stderr,"%d:set the time fail\n", errno);
            return -1;
     	  	}    
      	  //printf("修改参数为:%d\n", set_ret);
      	  if(status == 1){
      	  	//只记录稳定状态的offset
      	    sprintf(buf_log,"the average offset is %ld ns \n",AveOffset);
            write(fd_log,buf_log,strlen(buf_log));
            exp_count++;
      	  }
      	  flag = 0;
      	  count = 0;
        }else{
      	  count++;
      	  printf("waitng...%d\n",count);
      	  write_info(time_ns,1,fd_debug);
        }
        
        
//        write_info(time_ns,1,fd_log);
        sprintf(buf_log,"the average offset is %ld ns \n\n",AveOffset);
        write(fd_debug,buf_log,strlen(buf_log));
          
        if(exp_count == 1000){
        	return -1;
        }

      }
      
      close(fd_con);
      close(fd_debug);
      close(fd_log);
}


//从读取到的消息中，截取时间信息
void getTheTime( char* str, struct timespec *time){
    int len;
    (*time).tv_sec = 0;
    (*time).tv_nsec = 0;

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
                (*time).tv_nsec = (*time).tv_nsec*10 + str[i]-'0';
            }
        }
    }
}

//计算时间偏差
long getOffset(struct timespec t1, struct timespec t2, struct timespec t3, struct timespec t4){
  long offset = 0;
  offset = (t2.tv_sec*1000000000+t2.tv_nsec)-(t1.tv_sec*1000000000+t1.tv_nsec)-(t4.tv_sec*1000000000+t4.tv_nsec)+(t3.tv_sec*1000000000+t3.tv_nsec);
  offset = offset / 2;
  return offset;
}

//计算delay
long getDelay(struct timespec t1, struct timespec t2, struct timespec t3, struct timespec t4){
  long delay = 0;
  delay = (t2.tv_sec*1000000000+t2.tv_nsec)-(t1.tv_sec*1000000000+t1.tv_nsec)+(t4.tv_sec*1000000000+t4.tv_nsec)-(t3.tv_sec*1000000000+t3.tv_nsec);
  delay = delay / 2;
  return delay;
}

//计算平均时间偏差
long getAveOffset(struct timespec off[][4],int n){
  long time_offset[n];
  long offset = 0;
  int i;
  
  for( i=0 ; i<n ; i++ ){
  	time_offset[i] = getOffset(off[i][0],off[i][1],off[i][2],off[i][3]);
  }
  
  for( i=0 ; i<n ; i++ ){
    offset = offset + time_offset[i];
  }
  offset = offset / n;
  return offset;
}

//计算平均时间偏差
long getAveDelay(struct timespec off[][4],int n){
  long time_delay[n];
  long delay = 0;
  int i;
  
  for( i=0 ; i<n ; i++ ){
  	time_delay[i] = getDelay(off[i][0],off[i][1],off[i][2],off[i][3]);
  }
  
  for( i=0 ; i<n ; i++ ){
    delay = delay + time_delay[i];
  }
  delay = delay / n;
  return delay;
}

//设置时间
int setTheTime(long offset){ 
//  struct timeval time;
  struct timespec time_ns;
  struct timespec time_start,time_stop;
  long result;
  int ret;
  
//  offset = offset / 2 ;
  
  clock_gettime(CLOCK_REALTIME,&time_ns);
  
//  printf("1.settime sec:%ld,usec:%ld\n",time_ns.tv_sec,time_ns.tv_nsec);
  
  time_ns.tv_sec -= (offset) / 1000000000;
  time_ns.tv_nsec -= (offset) % 1000000000;
  
//  printf("2.settime sec:%ld,usec:%ld\n",time_ns.tv_sec,time_ns.tv_nsec);
  
  if(time_ns.tv_nsec >= 1000000000){
  	time_ns.tv_sec += time_ns.tv_nsec / 1000000000;
  	time_ns.tv_nsec = time_ns.tv_nsec % 1000000000 ;
  }else if(time_ns.tv_nsec <0){
  	time_ns.tv_sec -= time_ns.tv_nsec / 1000000000 + 1;
  	time_ns.tv_nsec = 1000000000 + time_ns.tv_nsec ;
  }

//  printf("3.settime sec:%ld,usec:%ld\n",time_ns.tv_sec,time_ns.tv_nsec);
  ret = clock_settime(CLOCK_REALTIME,&time_ns);
  return ret;
}


void write_info(struct timespec time[][4],int n,int fd_log){
  char buf_log[128];
  long diff1,diff2,offset,delay;
  
  for( int i=0 ; i<n ; i++ ){
  	
  	diff1 = (time[i][1].tv_sec*1000000000+time[i][1].tv_nsec)-(time[i][0].tv_sec*1000000000+time[i][0].tv_nsec);
  	diff2 = (time[i][3].tv_sec*1000000000+time[i][3].tv_nsec)-(time[i][2].tv_sec*1000000000+time[i][2].tv_nsec);
  	offset = (diff1 - diff2)/2;
  	delay = (diff1 + diff2)/2;
  
  	sprintf(buf_log,"t1:%ld,%ld\t" , time[i][0].tv_sec , time[i][0].tv_nsec);
  	write(fd_log,buf_log,strlen(buf_log));
  	sprintf(buf_log,"t2:%ld,%ld\t" , time[i][1].tv_sec , time[i][1].tv_nsec);
  	write(fd_log,buf_log,strlen(buf_log));
  	sprintf(buf_log,"t2-t1:%ld\n" , diff1);
  	write(fd_log,buf_log,strlen(buf_log));
  	
  	sprintf(buf_log,"t3:%ld,%ld\t" , time[i][2].tv_sec , time[i][2].tv_nsec);
  	write(fd_log,buf_log,strlen(buf_log));
  	sprintf(buf_log,"t4:%ld,%ld\t" , time[i][3].tv_sec , time[i][3].tv_nsec);
  	write(fd_log,buf_log,strlen(buf_log));
  	sprintf(buf_log,"t4-t3:%ld\n" , diff2);
  	write(fd_log,buf_log,strlen(buf_log));
  	sprintf(buf_log,"delay:%ld\t" , delay);
  	write(fd_log,buf_log,strlen(buf_log));
  	sprintf(buf_log,"offset:%ld\n" , offset);
  	write(fd_log,buf_log,strlen(buf_log));
  	
  }

}



#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdbool.h>

#include<time.h>
#include <pthread.h>
//#include "my_queue.h"
#define GET_MAP 1
#define REP_GET_MAP 2
#define PR_BUF 3
#define max_offset  33554432

#define packet_length 64 //块大小@
#define MAX_MSG_NUM 100 //队列最大长度 
#define PRIORITY_NUM 3 //优先级数目
#define max_vm 2 //最大主机数

unsigned char my_id = 3;
unsigned dest_id =  1;

unsigned char state=0; //用来表示当前的发送队列状态
unsigned int send_num = 0;

#define OFFSET_LOG "/home/zsr/Inter-VM-PTP/ivshmem/x86/offset_ns.log"
#define DEBUG_LOG "/home/zsr/Inter-VM-PTP/ivshmem/x86/debug_ns.log"

#pragma  pack(1)



struct pr_buff_packet{
    unsigned char src;
    unsigned char op;
    unsigned char len;
    unsigned int sec;
    unsigned int nsec;
    //unsigned char p[packet_length-3];
};

//存放共享内存的队列的信息

struct mesg 
{
  unsigned char dest;           //目的id
  struct mesg *next;            
  unsigned char text[packet_length];    //发送内容
  unsigned int len;                 //长度
  unsigned char priority;           //优先级,优先级越小，优先级越高
  //static unsigned char num;
};
struct mesg *head[PRIORITY_NUM],*mqend[PRIORITY_NUM],*empty_que_head, *empty_que_tail;//队头和队尾

struct packet_msg{
    unsigned char value;
    //unsigned char dest;
    //unsigned ychar src;
    unsigned char op; 
    unsigned int sec;
    unsigned int nsec;
    //unsigned char p[packet_length-2];
};

struct RWBUFF 
{
  int id;
  struct packet_msg *buff;
  unsigned int filesize;
  unsigned int offset;
 }txbuff[max_vm];


int insert_mesg(struct mesg *msg);
int send_msg(unsigned char *p,unsigned int length,unsigned char priority,unsigned char  dest);
int send_packet();

unsigned char * get_buff_map(unsigned int filesize,unsigned int offset);
int get_id();
void askforid(unsigned int filesize);
void * send_hander();

long getOffset(struct timespec t1, struct timespec t2, struct timespec t3, struct timespec t4);
long getAveOffset(struct timespec off[][4],int n);
int setTheTime(long offset);

void write_info(struct timespec time[][4],int n, int fd);

const char *filename = "/dev/ivshmem0";
unsigned int my_filesize =  0x10000;

unsigned int count =0;
unsigned char testchar[64];

struct timeval oldtime;


int main(int argc,char **argv)
{
    que_init();
    mmap_init();
    int result,i,t,num,ret;
    
    long time_offset[11];
    long time_aveoffset;
    char buf_log[128];
    int fd_offset,fd_debug;
    
    int max_num=my_filesize/packet_length;//块数目

    struct packet_msg *rev_msg; //接收块内存
    struct packet_msg time_msg;
//    struct pr_buff_packet *pbp;
    
//    struct timeval t1,t2,t3,t4;
//    struct timeval time[11][4];
    
    struct timespec t1,t2,t3,t4;
    struct timespec time[2][4];
    
    int status = 0;
    int count = 0;
    int flag = 1;

    rev_msg = txbuff[(my_id-1)/2].buff;
    for(i=0;i<max_num;i++)
       rev_msg[i].value=0;
       
    printf("my_id:%d\n",my_id);
    printf("my_offset:%d\n",txbuff[(my_id-1)/2].offset);

    pthread_t send_thread,rev_thread;

    result = pthread_create(&send_thread,NULL,send_hander,NULL);
    if (result= 0) {
        printf("发送线程创建失败");
        return 0;
    } 
    
    i=1; 
    if((fd_offset=open(OFFSET_LOG,O_CREAT|O_WRONLY|O_APPEND,0600))<0){
    	printf("open offset file err\n");
    	exit(0);
    }
    
    if((fd_debug = open(DEBUG_LOG, O_CREAT|O_WRONLY|O_APPEND,0600 )) <0 ){
    	printf("open debug file err\n");
    	exit(0);
    }
    
    while(1)
    {
    	num = 1;
    	while(num>=0){
    		
    		while(rev_msg[i].value%2!=0){
//                		gettimeofday(&t2,NULL);
//                		gettimeofday(&time[num][1],NULL);
                		clock_gettime(CLOCK_REALTIME,&time[num][1]);
                		if(rev_msg[i].op == 1){
                			//发包处理;
                			rev_msg[i].value = 0;
                			
                			time[num][0].tv_sec = rev_msg[i].sec;
                			time[num][0].tv_nsec = rev_msg[i].nsec;
                			
                			//t2 = time;
//                			printf("t1: %ld,%ld\n",time[num][0].tv_sec,time[num][0].tv_nsec);
//                			printf("t2: %ld,%ld\n",time[num][1].tv_sec,time[num][1].tv_nsec);
//        				gettimeofday(&t3,NULL);
					
//					gettimeofday(&time[num][2],NULL);
					clock_gettime(CLOCK_REALTIME,&time[num][2]);
//        				sprintf(testchar,"%c%c%c%ld,%ld",my_id+1,3,61,t3.tv_sec,t3.tv_usec);

					time_msg.value = my_id+1;
					time_msg.op = 3;
					time_msg.sec = time[num][2].tv_sec;
        				time_msg.nsec = time[num][2].tv_nsec;
        				
//	       			printf("t3: %ld,%ld\n",time[num][2].tv_sec,time[num][2].tv_nsec);
        				send_msg(&time_msg,64,2,1);
        				
                		}else if(rev_msg[i].op == 4){
                			
                			time[num][3].tv_sec = rev_msg[i].sec;
                			time[num][3].tv_nsec = rev_msg[i].nsec;
                			rev_msg[i].value = 0;
                			
//                			printf("t4: %ld,%ld\n",time[num][3].tv_sec,time[num][3].tv_nsec);
//                			time_offset[num] = getOffset(t1,t2,t3,t4);
//                			printf("---------------%d: %ld us--------------------\n",num,time_offset[num]);
                			num--;
                		}
                		break;
   
    		}
//    		i++;
//    		if(i>=max_num)i=0;
    		
    	}
//    	time_aveoffset = getAveOffset(time_offset,1);
	time_aveoffset = getAveOffset(time,1);
//	if( time_aveoffset > 1000 || time_aveoffset < -1000){

	if( count == 10 ){
		flag = 1;
		count = 0;

	}else if(time_aveoffset >= -100 && time_aveoffset <= 100){
		flag = 1;
		count = 0;
		status = 1;
	}else{
		status = 0;
	}
	
	if( flag == 1 ){
		printf("---------the average timeoffset is %ld ns----------\n",time_aveoffset);
		ret = setTheTime(time_aveoffset);
    		if(ret == -1){
    			fprintf(stderr,"%d:set the time fail\n",errno);
    			return -1;
    		}
    		if(status == 1){
    		    	sprintf(buf_log,"%ld\n",time_aveoffset);
    			write(fd_offset,buf_log,strlen(buf_log));
    		}
    		flag = 0;
    		count = 0;
	}else{
		count ++;
		printf("waiting...%d, %ldns \n",count,time_aveoffset);
	}
    	
    	write_info(time,1,fd_debug);
//    	}
    	
//    	close(fd_offset);
        //收包处理：
        //sleep(5)
    }
    close(fd_offset);
    close(fd_debug);
    return 0;
}





unsigned char * get_buff_map(unsigned int filesize, unsigned int offset)
{
    int fd;

    if((fd = open(filename, O_RDWR|O_NONBLOCK,0644))<0)
    {
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    unsigned char *map;
    if((  map = (char *)mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd,offset)) <= 0)
    {
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    close(fd);
    return map;
}




void mmap_init()
{
    //my_id = 3;
    //dest_id = 1;
    int i;
    i = (my_id-1)/2;
    txbuff[i].id = my_id;
    txbuff[i].filesize = my_filesize;
    txbuff[i].offset = my_filesize;
    txbuff[i].buff = get_buff_map (txbuff[i].filesize,txbuff[i].offset);

    i= (dest_id-1)/2;
    txbuff[i].id = dest_id;
    txbuff[i].filesize = my_filesize;
    txbuff[i].offset = 0;
    txbuff[i].buff = get_buff_map (txbuff[i].filesize,txbuff[i].offset);
          
   return ;
}

void * send_hander()
{
    while(1)
    {
        send_packet();
        sleep(5);
    }
}


//计算时间偏差
long getOffset(struct timespec t1, struct timespec t2, struct timespec t3, struct timespec t4){
  long long offset = 0;
  offset = (t2.tv_sec*1000000000+t2.tv_nsec)-(t1.tv_sec*1000000000+t1.tv_nsec)-(t4.tv_sec*1000000000+t4.tv_nsec)+(t3.tv_sec*1000000000+t3.tv_nsec);
  offset = offset / 2;
  return (long)offset;
}

//计算平均时间偏差
long getAveOffset(struct timespec off[][4],int n){
  long time_offset[n];
  long offset = 0;
  int i;
  
  for( i=0 ; i<n ; i++ ){
  	time_offset[i] = getOffset(off[i][0],off[i][1],off[i][2],off[i][3]);
//  	printf("%d, %ld\n",i,time_offset[i]);
  }
  
  for( i=0 ; i<n ; i++ ){
    offset = offset + time_offset[i];
  }
  offset = offset / n;
  return offset;
}

//设置时间
int setTheTime(long offset){ 
//  struct timeval time;
  struct timespec time_ns;
  int ret;
  
  clock_gettime(CLOCK_REALTIME,&time_ns);
  
//  printf("before setting: %ld,%ld\n",time_ns.tv_sec,time_ns.tv_nsec);
  
  time_ns.tv_sec -= (offset) / 1000000000;
  time_ns.tv_nsec -= (offset) % 1000000000;
  
//  printf("before setting: %ld,%ld\n",time_ns.tv_sec,time_ns.tv_nsec);
  
  if(time_ns.tv_nsec >= 1000000000){
  	time_ns.tv_sec += time_ns.tv_nsec / 1000000000;
  	time_ns.tv_nsec = time_ns.tv_nsec % 1000000000;
  }else if(time_ns.tv_nsec < 0){
  	time_ns.tv_sec -= (time_ns.tv_nsec / 1000000000 + 1);
  	time_ns.tv_nsec = 1000000000 + (time_ns.tv_nsec % 1000000000);
  }
  
//  printf("after setting: %ld,%ld\n",time_ns.tv_sec,time_ns.tv_nsec);
  
//  time.tv_sec = 0 - offset / 1000000;
//  time.tv_usec = 0 - offset % 1000000;
//  printf("*****sec: %ld, usec: %ld*****\n",-time.tv_sec,-time.tv_usec);
//  ret = adjtime(&time, &oldtime);
//  oldtime = time;

  ret = clock_settime(CLOCK_REALTIME,&time_ns);
  return ret;
}


void my_memcpy(unsigned char *d,unsigned char * s ,int length)
{
    int i;
    for(i=0;i<length;i++)
    d[i]=s[i];
}

int send_msg(unsigned char *p,unsigned int length,unsigned char priority,unsigned char  dest)
{   
    struct  mesg *msg;
    int i,n,j;
    //if(msg == NULL)return -1; 
    //发送部分
    msg = empty_que_head;
    i = (dest-1)/2;
    n= txbuff[i].filesize/packet_length;
    switch (priority)
    {
    case 0:
        if((state&0x01)==0){
            state= (state|0x01);
            for(int j=0;j<n;j++)
            {
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                txbuff[i].buff[j].value =my_id;
                if((state|0x01)==0x01&&head[0]->next==NULL)state=(state^0x01);
                return 1;
                }
            }   
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x01);
        if(head[0]->next==NULL){mqend[0] = msg; head[0]->next=msg;}
        else {mqend[0]->next = msg; mqend[0] = mqend[0]->next;}
        return 0;
        break;
    case 1:
        if((state&0x03)==0){
            state= (state|0x02);
            for(int j=0;j<n;j++)
            {
                if(txbuff[i].buff[j].value==0)
                {
                memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                txbuff[i].buff[j].value =my_id;
                if((state|0x02)==0x02&&head[1]->next==NULL)state=(state^0x02);
                return 1;
                }
            }   
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x02);
        if(head[1]->next==NULL){mqend[1] = msg; head[1]->next=msg;}
        else {mqend[1]->next = msg; mqend[1] = mqend[1]->next;}
        break;
    case 2:
        if((state&0x07)==0){
           state= (state|0x04);
//           j=send_num;
	   j=1;
             while(1){
//             	j++;
//            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                   memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
//                   printf("send_msg:ok,%d,%s\n",j,p);
                   txbuff[i].buff[j].value =my_id;
                   if(head[2]->next==NULL)state=(state&(~0x04));
//                   send_num=j;
                   return 1;
                }
            }
           printf("%d\n",j);
        }
        if(empty_que_head->next==NULL) return -1;
        empty_que_head = empty_que_head->next; 
        msg->next =NULL;
        msg->len = length;
        msg->dest = dest;
        msg->priority =priority;
        memcpy(msg->text, p,length);
        state= (state|0x04);
        if(head[2]->next==NULL){mqend[2] = msg; head[2]->next=msg;}
        else {mqend[2]->next = msg; mqend[2] = mqend[2]->next;}
        return 0;
        break;
    default:
        break;
    }
    return 0;
}



int send_packet()
{
    struct mesg *tmp;
    char *p;
    unsigned int i,k;
    unsigned int t;
    for(t=0;t<PRIORITY_NUM;t++)
    {
        if(head[t]->next!=NULL) break;
    }
    if(t==PRIORITY_NUM){state=0;//printf("NULL\n");
    return -1;}
    tmp=head[t]->next;
    i=(tmp->dest-1)/2;
    int n;
    n= txbuff[i].filesize/packet_length;
    for(int j=0;j<n;j++)
    {
        if(txbuff[i].buff[j].value==0)
        {
            //printf("\n");
            if(tmp->len == 0){  printf("error:length is zero\n");return 0;}
            txbuff[i].buff[j].value=my_id+1;
            memcpy((unsigned char*)&txbuff[i].buff[j],tmp->text,tmp->len);
            printf("send_packet:ok,%d\n",j);
            txbuff[i].buff[j].value = my_id;
            if(head[t]->next->next==NULL){mqend[t]=head[t]->next;}
            head[t]->next=head[t]->next->next;
            tmp->next==NULL;
            empty_que_tail->next=tmp;
            empty_que_tail = tmp;
            return j;
        }
    }
    return -1;
}

void que_init()
{
    struct  mesg *tmp;
    int i;
    state=0;
    empty_que_head = (struct mesg *)malloc(sizeof(struct  mesg));
    tmp =empty_que_head;
    for(i=0;i<MAX_MSG_NUM;i++)
    {
        tmp->next = (struct mesg *)malloc(sizeof(struct  mesg));
        tmp = tmp->next;
    }
     empty_que_tail = tmp;
     if(empty_que_tail==NULL){ printf("error");}
    for(i=0;i<PRIORITY_NUM;i++)
    {
        head[i]=(struct mesg *)malloc(sizeof(struct  mesg));
        head[i]->len = 0;
        //head[i]->text = NULL;
        head[i]->priority = 3;
        mqend[i]=head[i];
         //mqend[i]=NULL;
         //head[i]=NULL;
    }
    return;
}

void write_info(struct timespec time[][4],int n, int fd){
	char buf_log[128];
	long diff1,diff2,offset,delay;
	
	for(int i=0; i<n ; i++ ){
		diff1 = (time[i][1].tv_sec*1000000000 + time[i][1].tv_nsec) - (time[i][0].tv_sec*1000000000 + time[i][0].tv_nsec);
		diff2 = (time[i][3].tv_sec*1000000000 + time[i][3].tv_nsec) - (time[i][2].tv_sec*1000000000 + time[i][2].tv_nsec);
		offset = (diff1 - diff2)/2;
		delay = (diff1 + diff2)/2;
		
		sprintf(buf_log,"t1:%ld,%ld\t",time[i][0].tv_sec,time[i][0].tv_nsec);
		write(fd,buf_log,strlen(buf_log));
		sprintf(buf_log,"t2:%ld,%ld\t",time[i][1].tv_sec,time[i][1].tv_nsec);
		write(fd,buf_log,strlen(buf_log));
		sprintf(buf_log,"t2-t1:%ld\n",diff1);
		write(fd,buf_log,strlen(buf_log));
		
		sprintf(buf_log,"t3:%ld,%ld\t",time[i][2].tv_sec,time[i][2].tv_nsec);
		write(fd,buf_log,strlen(buf_log));
		sprintf(buf_log,"t4:%ld,%ld\t",time[i][3].tv_sec,time[i][3].tv_nsec);
		write(fd,buf_log,strlen(buf_log));
		sprintf(buf_log,"t4-t3:%ld\n",diff2);
		write(fd,buf_log,strlen(buf_log));
		
		sprintf(buf_log,"delay:%ld\n",delay);
		write(fd,buf_log,strlen(buf_log));
		sprintf(buf_log,"offset:%ld\n",offset);
		write(fd,buf_log,strlen(buf_log));
		
	}
}



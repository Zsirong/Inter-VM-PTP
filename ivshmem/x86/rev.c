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

#define OFFSET_LOG "/home/xiong/桌面/zsr/Inter-VM-PTP/ivshmem/x86/offset.log"

#pragma  pack(1)



struct pr_buff_packet{
    unsigned char src;
    unsigned char op;
    unsigned char len;
    unsigned int sec;
    unsigned int usec;
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
    unsigned int usec;
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

void getTheTime( char* str, struct timeval *time);
long getOffset(struct timeval t1, struct timeval t2, struct timeval t3, struct timeval t4);
long getAveOffset(struct timeval off[][4],int n);
int setTheTime(long offset);

const char *filename = "/dev/shm/lzh";
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
    int fd_offset;
    
    int max_num=my_filesize/packet_length;//块数目

    struct packet_msg *rev_msg; //接收块内存
    struct packet_msg time_msg;
//    struct pr_buff_packet *pbp;
    
    struct timeval t1,t2,t3,t4;
    struct timeval time[11][4];

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
    while(1)
    {
    	num = 10;
    	if((fd_offset=open(OFFSET_LOG,O_CREAT|O_WRONLY|O_APPEND,0600))<0){
    		printf("open file err\n");
    		exit(0);
    	}
    	while(num>=0){
    		
    		while(rev_msg[i].value%2!=0){
//                		gettimeofday(&t2,NULL);
                		gettimeofday(&time[num][1],NULL);
                		if(rev_msg[i].op == 1){
                			//发包处理;
//                			getTheTime(rev_msg[i].p,&t1);
                			rev_msg[i].value = 0;
                			
                			time[num][0].tv_sec = rev_msg[i].sec;
                			time[num][0].tv_usec = rev_msg[i].usec;
                			
                			//t2 = time;
//                			printf("t1: %ld,%ld\n",time[num][0].tv_sec,time[num][0].tv_usec);
//                			printf("t2: %ld,%ld\n",time[num][1].tv_sec,time[num][1].tv_usec);
//        				gettimeofday(&t3,NULL);
					
					gettimeofday(&time[num][2],NULL);
//        				sprintf(testchar,"%c%c%c%ld,%ld",my_id+1,3,61,t3.tv_sec,t3.tv_usec);

					time_msg.value = my_id+1;
					time_msg.op = 3;
					time_msg.sec = time[num][2].tv_sec;
        				time_msg.usec = time[num][2].tv_usec;
        				
//	       			printf("t3: %ld,%ld\n",time[num][2].tv_sec,time[num][2].tv_usec);
        				send_msg(&time_msg,64,2,1);
        				
                		}else if(rev_msg[i].op == 4){
//                			getTheTime(rev_msg[i].p,&t4);
                			
                			time[num][3].tv_sec = rev_msg[i].sec;
                			time[num][3].tv_usec = rev_msg[i].usec;
                			rev_msg[i].value = 0;
                			
//                			printf("t4: %ld,%ld\n",t4.tv_sec,t4.tv_usec);
//                			time_offset[num] = getOffset(t1,t2,t3,t4);
//                			printf("---------------%d: %ld us--------------------\n",num,time_offset[num]);
                			num--;
                		}
                		break;
   
    		}
//    		i++;
//    		if(i>=max_num)i=0;
    		
    	}
//    	time_aveoffset = getAveOffset(time_offset,10);
	time_aveoffset = getAveOffset(time,10);
    	printf("---------the average timeoffset is %ld----------\n",time_aveoffset);
    	sprintf(buf_log,"%ld\n",time_aveoffset);
    	write(fd_offset,buf_log,strlen(buf_log));
    	ret = setTheTime(time_aveoffset);
    	if(ret == -1){
    		fprintf(stderr,"%d:set the time fail\n",errno);
    		return -1;
    	}
    	close(fd_offset);
    	sleep(5);
        //收包处理：
        //sleep(5)
    }
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


//从读取到的消息中，截取时间信息
void getTheTime( char* str, struct timeval *time){
    int len;
    (*time).tv_sec = 0;
    (*time).tv_usec = 0;

    len = strlen(str);
    for(int i=1; i<len; i++)
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
  long long offset = 0;
  offset = (t2.tv_sec*1000000+t2.tv_usec)-(t1.tv_sec*1000000+t1.tv_usec)-(t4.tv_sec*1000000+t4.tv_usec)+(t3.tv_sec*1000000+t3.tv_usec);
  offset = offset / 2;
  return (long)offset;
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
  time.tv_sec = 0 - offset / 1000000;
  time.tv_usec = 0 - offset % 1000000;
  printf("*****sec: %ld, usec: %ld*****\n",time.tv_sec,time.tv_usec);
  ret = adjtime(&time, &oldtime);
  oldtime = time;
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

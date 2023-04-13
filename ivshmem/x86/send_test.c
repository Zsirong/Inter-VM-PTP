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
unsigned char my_id = 1;
unsigned char dest_id =  3;

unsigned char state=0; //用来表示当前的发送队列状态
unsigned int send_num = 0;

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


unsigned char * get_buff_map(unsigned int filesize,unsigned int offset);
int get_id();
void askforid(unsigned int filesize);
void * send_hander();

int insert_mesg(struct mesg *msg);
int send_msg(unsigned char *p,unsigned int length,unsigned char priority,unsigned char  dest);
int send_packet();

const char *filename = "/dev/shm/zsr";
unsigned int my_filesize =  0x10000;

unsigned int count =0;
unsigned char testchar[64];


int main(int argc,char **argv)
{
    que_init();
    mmap_init();
    int time_send,result,i,t,num;
    int max_num=my_filesize/packet_length;//块数目

    struct packet_msg *rev_msg; //接收块内存
    struct packet_msg time_msg;
//    struct pr_buff_packet *pbp;
    
//    struct timeval time;
   
    struct timespec time; 
    
    rev_msg = txbuff[(my_id-1)/2].buff;
    for(i=0;i<max_num;i++)
       rev_msg[i].value=0;
    printf("my_id:%d\n",my_id);
    printf("my_offset:%d\n",txbuff[(my_id-1)/2].offset);

//    testchar[1]=0;
    
    time_send = 1;
    
    pthread_t send_thread,rev_thread;
    
    result = pthread_create(&send_thread,NULL,send_hander,NULL);
    if (result= 0) {
        printf("发送线程创建失败");
        return 0;
    } //*/
    
    while(1)
    {
    	num = 1;
    	while(num>=0){
    		//收包处理
    		i=1;
		if( time_send == 1 ){     //true is the first send
//    			gettimeofday(&time,NULL);
    			clock_gettime(CLOCK_REALTIME,&time);
//    			sprintf(testchar,"%c%c%c%ld,%ld",my_id+1,1,61,time.tv_sec,time.tv_usec);
			
			time_msg.value = my_id+1;
        		time_msg.op = 1;
        		time_msg.sec = time.tv_sec;
        		time_msg.nsec = time.tv_nsec;
        		
    			send_msg(&time_msg,64,2,3);    			
    			time_send = 4;     //false is the fourth send

//    			printf("t1: %ld,%ld\n",time.tv_sec,time.tv_nsec);
    		}
    		while(rev_msg[i].value%2!=0){
//    			printf("i=%d,value=%d\n",i,rev_msg[i].value);

    				//printf("rev:%d,%d,%d\n",num,rev_msg[i].value,rev_msg[i].op);
        			//printf("rev:%s\n",rev_msg[i].p);
//        			gettimeofday(&time,NULL);
				clock_gettime(CLOCK_REALTIME,&time);
        			rev_msg[i].value = 0;
        			
        			//发包处理,收到消息才立刻发送消息
				time_msg.value = my_id+1;
        			time_msg.op = 4;
        			time_msg.sec = time.tv_sec;
        			time_msg.nsec = time.tv_nsec;
        			
//        			printf("t4: %ld,%ld\n",time.tv_sec,time.tv_nsec);
        			send_msg(&time_msg,64,2,3);
				time_send = 1;
				
        			num--;
        			break;

    			}
    			
        	
    	}
//        sleep(10);
    }
    return 0;
}



unsigned char * get_buff_map(unsigned int filesize, unsigned int offset)
{
    int fd;

    if((fd = open(filename, O_RDWR,0644))<0)
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
    //my_id=1;
    //dest_id=3;
    int i;
    i = (my_id-1)/2;
    txbuff[i].id = my_id;
    txbuff[i].filesize = my_filesize;
    txbuff[i].offset = 2 * my_filesize;
    txbuff[i].buff = get_buff_map (txbuff[i].filesize,txbuff[i].offset);

    i= (dest_id-1)/2;
    txbuff[i].id = dest_id;
    txbuff[i].filesize = my_filesize;
    txbuff[i].offset = 3 * my_filesize;
    txbuff[i].buff = get_buff_map (txbuff[i].filesize,txbuff[i].offset);
          
   return ;
}

void * send_hander()
{

    while(1)
    {
        send_packet();
        //sleep(5);
    }
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
//                   printf("send_msg:ok,%d,%s,id:%d\n",j,p,my_id);
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


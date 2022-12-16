#ifndef _MY_QUEUE_H_
#define _MY_QUEUE_H_
#define packet_length 64 //块大小@
#define MAX_MSG_NUM 100 //队列最大长度 
#define PRIORITY_NUM 3 //优先级数目
#define max_vm 2 //最大主机数
unsigned char my_id = 1;
unsigned dest_id =  3;

unsigned char state=0; //用来表示当前的发送队列状态
unsigned int send_num = 0;

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
    unsigned char p[packet_length-2];
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
           j=send_num;
             while(1){
             	j++;
            	if(j>=n)j=0;
                if(txbuff[i].buff[j].value==0)
                {
                   memcpy((unsigned char*)&txbuff[i].buff[j],p,length);
                   printf("send_msg:ok,%d,%s\n",j,p);
                   txbuff[i].buff[j].value =my_id;
                   if(head[2]->next==NULL)state=(state&(~0x04));
                   send_num=j;
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

#endif

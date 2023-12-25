/*

*/
#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

//struct lock
struct rwlock_t
{
    sem_t lock;//read lock
    sem_t wlock;//write lock
    sem_t lrulock;
    int readCnt;
};

struct CACHE
{
    char url[MAXLINE];
    int lru;
    size_t len;
    char content[MAX_OBJECT_SIZE];
};

struct rwlock_t *rw;
struct CACHE cache[MAX_CACHE]; 
int lru = 1;

void doit(int clientfd);//处理请求行
int parse_url(char *url,char *hostname,char *path,char *port,char *request_head);//解析请求行
void read_requesthdrs(rio_t *rp,char *request_head,char *hostname);//读报头并发给server
void return_content(int serverfd, int clientfd, char *url);//像客户端传递服务器返回值，并写入cache
void *thread(void *vargp);
void rwlock_init();//初始化锁
int read_cache(char *url,char *content);//读cache
void write_cache(char *url,char *buf, size_t length);//写cache
void cache_init();//初始化cache

void cache_init()
{
    for(int i=0;i<MAX_CACHE;i++)
    {
        cache[i].lru=0;
        cache[i].len=0;
    }
    return;
}

void write_cache(char *url,char *buf,size_t length)
{
    P(&(rw->wlock));//加写者锁

    //找lru最小cache
    int i = 0;
    int min = cache[0].lru;

    for(int j=1;j<MAX_CACHE;j++)
    {
        if(cache[j].lru<min)
        {
            min=cache[j].lru;
            i=j;
        }
    }

    //更改lru，加锁
    P(&(rw->lrulock));
    lru++;
    V(&(rw->lrulock));

    //修改cache内容
    cache[i].lru = lru++;
    strcpy(cache[i].url,url);
    memcpy(cache[i].content,buf,length);
    cache[i].len = length;

    //释放写者锁
    V(&(rw->wlock));

    return;
}

int read_cache(char *url,char *content)
{
    //修改readCnt，加读者锁
    P(&(rw->lock));
    rw->readCnt++;

    //读时不允许有写者
    if(rw->readCnt==1)
        P(&(rw->wlock));

    //释放读者锁
    V(&(rw->lock));

    //查找cache是否命中
    int length = -1;
    for(int i=0;i<MAX_CACHE;i++)
    {
        if(strcmp(url,cache[i].url) == 0)
        {
            P(&(rw->lrulock));
            lru++;
            V(&(rw->lrulock));

            cache[i].lru = lru++;
            memcpy(content,cache[i].content,cache[i].len);
            length = cache[i].len;
            break;
        }
    }

    //读完，修改readCnt，加读者锁
    P(&(rw->lock));
    rw->readCnt--;

    //如果已经没有读者了，释放写者锁
    if(rw->readCnt==0)
        V(&(rw->wlock));

    //释放读者锁
    V(&(rw->lock));

    return length;
}

void rwlock_init()
{
    rw->readCnt=0;
    Sem_init(&(rw->lock),0,1);
    Sem_init(&(rw->wlock),0,1);
    Sem_init(&(rw->lrulock),0,1);
    return;
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

void return_content(int serverfd, int clientfd, char *url)
{
    size_t n;
    size_t size=0;
    char buf[MAXLINE];
    char content[MAX_OBJECT_SIZE];
    rio_t srio;

    Rio_readinitb(&srio,serverfd);
    while((n = Rio_readlineb(&srio,buf,MAXLINE)) != 0)
    {
        if (n == -1)
        {
            return;
        }

        Rio_writen(clientfd,buf,n);

        if(size+n<=MAX_OBJECT_SIZE)//判断cache是否存的下返回内容
        {
            memcpy(content+size,buf,n);
            size+=n;
        }
        else
        {
            size = MAX_OBJECT_SIZE+1;
        }

    }

    if(size<=MAX_OBJECT_SIZE)//若cache空间足够，写cache
        write_cache(url,content,size);

    return;
}

void read_requesthdrs(rio_t *rp,char *request_head,char *hostname)
{
    char buf[MAXLINE];
    char tmp[MAXLINE];

    for(Rio_readlineb(rp,buf,MAXLINE);strcmp(buf,"\r\n");Rio_readlineb(rp,buf,MAXLINE))
    {
        if(strncmp("Host",buf,4) == 0)//以报头中的Host信息为首选
        {
            char *end = strchr(buf+6,':');
            if(end==NULL)
                end = strstr(buf,"\r\n");

            char tmp2 = *end;

            *end = '\0';

            strcpy(hostname,buf+6);

            *end = tmp2;

            continue;
        }
        if(strncmp("User-Agent",buf,10) == 0 || strncmp("Connection",buf,10) == 0 || strncmp("Proxy-Connection",buf,16) == 0)
            continue;

        //printf("%s",buf);

        sprintf(tmp,"%s",request_head);
        sprintf(request_head,"%s%s",tmp,buf);//报头信息加入request_head
    }

    //添加必备报头
    sprintf(buf, "Host: %s\r\n", hostname);
    sprintf(tmp,"%s",request_head);
    sprintf(request_head,"%s%s",tmp,buf);

    sprintf(buf, "%s", user_agent_hdr);
    sprintf(tmp,"%s",request_head);
    sprintf(request_head,"%s%s",tmp,buf);

    sprintf(buf, "Connection: close\r\n");
    sprintf(tmp,"%s",request_head);
    sprintf(request_head,"%s%s",tmp,buf);

    sprintf(buf, "Proxy-Connection: close\r\n");
    sprintf(tmp,"%s",request_head);
    sprintf(request_head,"%s%s",tmp,buf);

    //加末尾空行
    sprintf(tmp,"%s",request_head);
    sprintf(request_head,"%s\r\n",tmp);

    return;
}

int parse_url(char *url,char *hostname,char *path,char *port,char *request_head)
{
    sprintf(port,"80"); 

    int len = strlen("http://");
    if (strncmp(url, "http://", len))
    {
        if(strncmp(url, "https://", len+1)==0)
        {
            len++;            
        }
        else
        {
            len = 0;
        }
    }

    char *Port,*Uri;

    Port = strchr(url+len,':');
    Uri = strchr(url+len,'/');

    if(!Uri)
        return -1;

    *Uri = '\0';
//有port读port
    if(Port)
    {
        *Port = '\0';
        strcpy(port,Port+1);
    }
//读hostname
    strcpy(hostname,url+len);
//读path
    *path = '/';
    strcpy(path+1,Uri+1);

    sprintf(request_head,"GET %s HTTP/1.0\r\n",path);
//复原url
    *Uri = '/';
    if(Port)
        *Port = ':';

    return 1;
}

void doit(int clientfd)
{
    char buf[MAX_OBJECT_SIZE],method[MAXLINE],url[MAX_OBJECT_SIZE],version[MAXLINE];
    char hostname[MAXLINE],path[MAXLINE],port[MAXLINE],request_head[MAXLINE];
    int serverfd;
    rio_t rio;
    char content[MAX_OBJECT_SIZE];
    ssize_t n;

    Rio_readinitb(&rio,clientfd);
    Rio_readlineb(&rio,buf,MAX_OBJECT_SIZE);

    printf("%s",buf);

    if(sscanf(buf,"%s %s %s",method,url,version)!=3)
        return ;

    if(strlen(url)>=MAXLINE)
    {
        printf("too long");
        return;
    }

    //method错误，返回
    if(strcasecmp(method,"GET"))
    {
        printf("Not implemented\n");
        return;
    }

    //若cache中存了，直接从cache读
    if((n = read_cache(url,content))!=-1)
    {
        Rio_writen(clientfd,content,n);
        return;
    }

    //解析请求行
    if(parse_url(url,hostname,path,port,request_head)<0)
        return;

    //获取报头
    read_requesthdrs(&rio,request_head,hostname);

    serverfd = Open_clientfd(hostname,port);

    if(serverfd<0)
    {
        printf("Connection failed\n");
        return;
    }

    Rio_writen(serverfd,request_head,strlen(request_head));

    return_content(serverfd,clientfd,url);

    Close(serverfd);

    return;
}

int main(int argc,char **argv)
{
    int listenfd,*connfd;
    char hostname[MAXLINE],port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if(argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    rw = Malloc(sizeof(struct rwlock_t));
    rwlock_init();

    listenfd = Open_listenfd(argv[1]);

    Signal(SIGPIPE, SIG_IGN);

    cache_init();

    while(1)
    {
        clientlen = sizeof(clientaddr);

        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);

        Getnameinfo((SA *)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
        printf("Accepted connection from (%s, %s)\n",hostname,port);

        Pthread_create(&tid,NULL,thread,connfd);
    }

    Close(listenfd);
    return 0;
}
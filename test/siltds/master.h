#ifndef _MASTER_H_
#define _MASTER_H_

#include <iostream>
#include "fawnds_types.h"
#include "value.h"

/* About thread */
#define THREAD_POOL_SIZE 10
#define SBUFSIZE 500

using namespace std;

typedef struct {
    int *buf;          /* Buffer array */
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

void *request_handler(void *vargp);
void process_conn(int browserfd);
sbuf_t sbuf; /* Shared buffer of connected descriptors */



#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct cacheobject_t {
    struct cacheobject_t *prev, *next;
    size_t size;
    char *uri;
    void *data;
};
typedef struct cacheobject_t CacheObject;

typedef struct {
    CacheObject *head, *tail;
    size_t size;
    sem_t mutex, w;
    int readcnt;
} cache_t;

cache_t cache;

void cache_init();
CacheObject *cache_get(char *uri);
int cache_insert(char *uri, void *data, size_t objectsize);
void cache_evict();

void print_cache();

#define PORT 4647
#define HTTP_PORT 80


namespace silt {
    
    class Master
    {
    private:
        /* Variables */
        string _myIP;
        int _myPort;
        typedef struct sockaddr SA;
        
        int open_listenfd();
        
        typedef struct {
            int *buf;          /* Buffer array */
            int n;             /* Maximum number of slots */
            int front;         /* buf[(front+1)%n] is first item */
            int rear;          /* buf[rear%n] is last item */
            sem_t mutex;       /* Protects accesses to buf */
            sem_t slots;       /* Counts available slots */
            sem_t items;       /* Counts available items */
        } sbuf_t;

        sbuf_t sbuf;
        
        void sbuf_init(sbuf_t *sp, int n);
        void sbuf_deinit(sbuf_t *sp);
        void sbuf_insert(sbuf_t *sp, int item);
        int sbuf_remove(sbuf_t *sp);



        static void *request_handler(void *vargp);
        
        
    public:
        explicit Master(string myIP, int myPort);
        ~Master();
        FawnDS_Return Listen();
        FawnDS_Return Put(const ConstValue& key, const ConstValue& data);
        FawnDS_Return Append(Value& key, const ConstValue& data);
        FawnDS_Return Get(const ConstValue& key, Value& data);
        FawnDS_Return Delete(const ConstValue& key);
    };
}



#endif
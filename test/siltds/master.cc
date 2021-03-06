#include <iostream>
#include <stdio.h>
#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <assert.h>

#include "master.h"
#include "fawnds_factory.h"
#include "fawnds_types.h"
#include "fawnds.h"
#include "csapp.h"
#include "hashutil.h"
#include "print.h"


using namespace std;
using namespace fawn;

/************************** 
 * Error-handling functions
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char *msg) /* posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void dns_error(char *msg) /* dns-style error */
{
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(0);
}

void app_error(char *msg) /* application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

/*********************************************************************
 * The Rio package - robust I/O functions
 **********************************************************************/
/*
 * rio_readn - robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char *)usrbuf;

    while (nleft > 0) {
  if ((nread = read(fd, bufp, nleft)) < 0) {
      if (errno == EINTR) /* interrupted by sig handler return */
    nread = 0;      /* and call read() again */
      else
    return -1;      /* errno set by read() */ 
  } 
  else if (nread == 0)
      break;              /* EOF */
  nleft -= nread;
  bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *)usrbuf;

    while (nleft > 0) {
  if ((nwritten = write(fd, bufp, nleft)) <= 0) {
      if (errno == EINTR)  /* interrupted by sig handler return */
    nwritten = 0;    /* and call write() again */
      else
    return -1;       /* errorno set by write() */
  }
  nleft -= nwritten;
  bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* refill if buf is empty */
  rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
         sizeof(rp->rio_buf));
  if (rp->rio_cnt < 0) {
      if (errno != EINTR) /* interrupted by sig handler return */
    return -1;
  }
  else if (rp->rio_cnt == 0)  /* EOF */
      return 0;
  else 
      rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
  cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = (char *)usrbuf;
    
    while (nleft > 0) {
  if ((nread = rio_read(rp, bufp, nleft)) < 0) {
      if (errno == EINTR) /* interrupted by sig handler return */
    nread = 0;      /* call read() again */
      else
    return -1;      /* errno set by read() */ 
  } 
  else if (nread == 0)
      break;              /* EOF */
  nleft -= nread;
  bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = (char *)usrbuf;

    for (n = 1; n < maxlen; n++) { 
  if ((rc = rio_read(rp, &c, 1)) == 1) {
      *bufp++ = c;
      if (c == '\n')
    break;
  } else if (rc == 0) {
      if (n == 1)
    return 0; /* EOF, no data read */
      else
    break;    /* EOF, some data was read */
  } else
      return -1;    /* error */
    }
    *bufp = 0;
    return n;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
  unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
  unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
} 

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
  unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
  unix_error("Rio_readlineb error");
    return rc;
} 

int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  return -1;
 
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int)) < 0)
  return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
  return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
  return -1;
    return listenfd;
}

/* Create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = (int *)calloc(n, sizeof(int));
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */   
    sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
    
}


/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}


/* Insert item onto the rear of shared buffer sp */

void sbuf_insert(sbuf_t *sp, int item)
{
    printf("[Thread %u] :sbuf_insert: sem_wait(&sp->slots)\n", (unsigned int)pthread_self());
    sem_wait(&sp->slots);                          /* Wait for available slot */
    printf("[Thread %u] :sbuf_insert: sem_wait(&sp->mutex)\n", (unsigned int)pthread_self());
    sem_wait(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    printf("[Thread %u] :sbuf_insert: sem_post(&sp->mutex)\n", (unsigned int)pthread_self());
    sem_post(&sp->mutex);                          /* Unlock the buffer */
    printf("[Thread %u] :sbuf_insert: sem_post(&sp->items)\n", (unsigned int)pthread_self());
    sem_post(&sp->items);                          /* Announce available item */
}


/* Remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t *sp)
{
    
    int item;
    printf("[Thread %u] :sbuf_remove: sem_wait(&sp->items)\n", (unsigned int)pthread_self());
    sem_wait(&sp->items);                          /* Wait for available item */
    printf("[Thread %u] :sbuf_remove: sem_wait(&sp->mutex)\n", (unsigned int)pthread_self());
    sem_wait(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    printf("[Thread %u] :sbuf_remove: sem_post(&sp->mutex)\n", (unsigned int)pthread_self());
    sem_post(&sp->mutex);                          /* Unlock the buffer */
    printf("[Thread %u] :sbuf_remove: sem_post(&sp->slots)\n", (unsigned int)pthread_self());
    sem_post(&sp->slots);                          /* Announce available slot */
    return item;
}

void cache_init() {
  sem_init(&cache.mutex, 0, 1);
  sem_init(&cache.w, 0, 1);
  cache.head = cache.tail = NULL;
  cache.size = 0;
  cache.readcnt = 0;
}

CacheObject *cache_get(char *key) {
  CacheObject *ret_obj = NULL;
  printf("[Thread %u] :cache_get: sem_wait(&cache.mutex)\n", (unsigned int)pthread_self());
  sem_wait(&cache.mutex);
  cache.readcnt++;
  if (cache.readcnt == 1) {
      printf("[Thread %u] :cache_get: sem_wait(&cache.w)\n", (unsigned int)pthread_self());
    sem_wait(&cache.w);
  }
  printf("[Thread %u] :cache_get: readcnt = %d\n", (unsigned int)pthread_self(), cache.readcnt);
    printf("[Thread %u] :cache_get: sem_post(&cache.mutex)\n", (unsigned int)pthread_self());
  sem_post(&cache.mutex);

  CacheObject *ptr = cache.head;
  cout << "~~~~~~~ Looking for key " << bytes_to_hex(key) << endl;
  while (ptr != NULL) {
    cout << "\tLooking cache key = " << bytes_to_hex(ptr->hash) << endl;

    if (ptr->next != NULL){
      /* Compare the two nodes, if key is in range, insert it between */
      if( (strcasecmp(key, ptr->hash) >= 0) &&
         (strcasecmp(key, ptr->next->hash) < 0) ){
          ret_obj = ptr;
          break;
      }  
    }else{
      if (ptr->prev == NULL){ // First node
          ret_obj = ptr;
          break;
      }else{ // last node
          if (strcasecmp(key, ptr->hash) >= 0){
            ret_obj = ptr;
            break;
          }else{
            ret_obj = ptr->prev;
            break;
          }
      }
    }
    
    ptr = ptr->next;
  }
  
  if (ret_obj != NULL)
    cout << "\tfound key " << bytes_to_hex(key) << " at nodeid " << bytes_to_hex(ptr->hash) << endl;
  else
    cout << "##### db missed" << endl;

  printf("[Thread %u] :cache_get: sem_wait(&cache.mutex)\n", (unsigned int)pthread_self());
  sem_wait(&cache.mutex); /* Lock mutex */
  cache.readcnt--;
  if (cache.readcnt == 0) {
      printf("[Thread %u] :cache_get: sem_post(&cache.w)\n", (unsigned int)pthread_self());
    sem_post(&cache.w);
  }
  printf("[Thread %u] :cache_get: readcnt = %d\n", (unsigned int)pthread_self(), cache.readcnt);
  sem_post(&cache.mutex); /* Unlock mutex */
  printf("[Thread %u] :cache_get: sem_post(&cache.mutex)\n", (unsigned int)pthread_self());

  assert(ret_obj != NULL);

  return ret_obj;
}

/* Assume that there's no object with the same nodeid in the cache */
int cache_insert(char *nodeid, size_t key_len) {

  /* Create a new cache object. Copy data to the cache object's */
  CacheObject *newobject = (CacheObject *)calloc(1, sizeof(CacheObject));
  newobject->hash = (char *)calloc(1, MAX_KEY_LENGTH);
  newobject->nodeid = (char *)calloc(1, strlen(nodeid));

  string key = HashUtil::MD5Hash(nodeid, MAX_KEY_LENGTH);
  char *ckey = strdup(key.c_str());
  memcpy(newobject->nodeid, nodeid, strlen(nodeid));
  memcpy(newobject->hash, ckey, MAX_KEY_LENGTH);
  
  printf("[Thread %u] :insert: sem_wait(&cache.w)\n", (unsigned int)pthread_self());
  sem_wait(&cache.w); /* Lock writer */

  /* There is no node in the list */
  if (cache.head == NULL && cache.tail == NULL) {
    cache.head = newobject;
    cache.tail = newobject;
    newobject->prev = NULL;
    newobject->next = NULL;
    // printf( %s\n", nodeid);
    cout << "~~~~~~~ init a new node: " << nodeid << " : " << bytes_to_hex(key) << endl;
  } else {
    /* Seach for the node that has nodeid < the given nodeid and 
                  the next nodeid > the given nodeid */
    CacheObject *ptr = cache.head;
      cout<< "~~~~~~~ Looking for hash = " << bytes_to_hex(newobject->hash) << endl;
      while (ptr != NULL) {
        cout<< "\tLooking for hash = " << bytes_to_hex(ptr->hash) << endl;
        /* Look for a cache object with the same nodeid.
          When found, move this object to the tail.
          Then return this object. */
        if (ptr->next != NULL){ // More than 2 nodes exist 
            CacheObject *next_node = ptr->next;
            /* Compare the two nodes, if nodeid is in range, insert it between */
            if( (strcasecmp(ckey, ptr->hash) > 0) &&
               (strcasecmp(ckey, next_node->hash) < 0) )
            {
                ptr->next = newobject;
                newobject->prev = ptr;
                newobject->next = next_node;
                next_node->prev = newobject;
                printf("\tinsert %s between\n", nodeid);
                break;
            }

        }else{ // One node exists
          // if (newobject->hash.compare(ptr->hash) > 0){
          if (strcasecmp(ckey, ptr->hash) > 0){
            // insert after the ptr node
            newobject->next = ptr->next;
            ptr->next = newobject;
            newobject->prev = ptr;

            if (cache.tail == ptr){
              cache.tail = newobject;
            }

            cout << "\tInsert after " << bytes_to_hex(ptr->hash) << endl;
            break;
          // }else if (newobject->hash.compare(ptr->hash) < 0 ){
          }else if (strcasecmp(ckey, ptr->hash) < 0){
            // insert before the ptr node
            newobject->prev = ptr->prev;
            newobject->next = ptr;
            ptr->prev->next = newobject;
            ptr->prev = newobject;

            // change the cache head
            if (cache.head == ptr){
              cache.head = newobject;  
            }

            cout << "\tInsert before " << bytes_to_hex(ptr->hash) << endl;
            break;
          }else{
            // Replace the old node
            newobject->next = ptr->next;
            newobject->prev = ptr->prev;
            // change the cache head
            if (cache.head == ptr){
              cache.head = newobject;  
            }
            if (cache.tail == ptr){
              cache.tail = newobject;
            }
            cout << "\tReplace existing node " << bytes_to_hex(ptr->hash) << endl;
            break;
          }
        }

        ptr = ptr->next;
      }

  }
  /* Update cache size */

  printf("[Thread %u] :insert: sem_post(&cache.w)\n", (unsigned int)pthread_self());
  sem_post(&cache.w); /* Unlock writer */

  print_cache();

  return 0;
}

/* Remove the first object in the cache. Then, decrease the cache size */
void cache_evict() {
  if (cache.head == NULL)
    return;

  printf("[Thread %u] :evict: sem_wait(&cache.w)\n", (unsigned int)pthread_self());
  sem_wait(&cache.w); /* Lock writer */

  CacheObject *victim = cache.head;
  cache.head = victim->next;
  (cache.head)->prev = NULL;
  // free(victim->hash);
  free(victim);


  printf("[Thread %u] :evict: sem_post(&cache.w)\n", (unsigned int)pthread_self());
  sem_post(&cache.w); /* Unlock writer */
}

void print_cache() {
  printf("** Cache (size = %u) **\n", (unsigned int)cache.size);
  CacheObject *ptr = cache.head;
  while (ptr != NULL) {
    cout << "\t" << bytes_to_hex(ptr->hash) << endl;
    ptr = ptr->next;
  }
  printf("** end **\n");
}

void *request_handler(void *vargp){
    
    pthread_detach(pthread_self());

    /* Wait for the job, which is a browser's file descriptor in s_buf
        After this thread got the browserfd, it then process it, close it,
        and finally wait for the other one. Do this forever */
    while (1) {
        int browserfd = sbuf_remove(&sbuf);
        printf("[Thread %u] is handling browserfd = %d\n", (unsigned int)pthread_self(), browserfd);
        process_conn(browserfd);
        close(browserfd);
        printf("    [Thread %u] has finished the job.\n", (unsigned int)pthread_self());
    }
    
    /* The thread never reaches here because of the while loop */
    return NULL;
}

/* 
 * process_conn - Process the connection
 * This function process the connection's request to GET data on the web server.
 */
void process_conn(int browserfd) {
    char buf[MAXLINE], method[MAXLINE], key[MAXLINE], value[MAXLINE];
    char host[MAXLINE], path[MAXLINE], cachebuf[MAX_OBJECT_SIZE], headerbuf[MAXLINE];
    rio_t browser_rio, webserver_rio;
    int webserverfd, n, is_exceeded_max_object_size;
    size_t cachebuf_size, headerbuf_size;

    struct hostent *hp;
    char *haddrp;
    struct sockaddr_in clientaddr;

    Rio_readinitb(&browser_rio, browserfd);
    while((n = Rio_readlineb(&browser_rio, buf, MAXLINE)) != 0){
      sscanf(buf, "%s %s %s", method, key, value);
      // printf("[Thread %u] <=== %s", (unsigned int)pthread_self(), buf);

      if (!strcasecmp(method, "CONN")) {
        // SILT request to join the cluster
        // Update cache table
        printf("CONN request from SILT: id=%s\n", key);
        
        // printf("dbid = %s\n", nodeid_hash);
        // nodeid_hash->printValue();
        int success = 0;
        char *ret_desc;
        if ((success = cache_insert(key, MAX_KEY_LENGTH)) > 0){
          ret_desc = "failed to connect the master\r\n";
        }else{
          ret_desc = "connected to the master\r\n";
        }
        Rio_writen(browserfd, ret_desc, strlen(ret_desc));
        cout << "Write result back to the client: " << ret_desc << endl;

        ret_desc = "PUT 12345678901234567890 1234\r\n";
        Rio_writen(browserfd, ret_desc, strlen(ret_desc)); 
        ret_desc = "GET 12345678901234567890\r\n";
        Rio_writen(browserfd, ret_desc, strlen(ret_desc)); 
      }else if (strcasecmp(method, "GET") == 0) {
        printf("GET request to SILT\n");
        // Look up for SILT node id
        string hash = HashUtil::MD5Hash(key, MAX_KEY_LENGTH);
        char *ckey = strdup(hash.c_str());
        CacheObject *cacheobj = cache_get(ckey);
        if (cacheobj != NULL) {
            /* Serve this cached object to the browser right away */
            // Rio_writen(browserfd, cacheobj->data, cacheobj->size);
            printf("@@@@@ Served nodeis %s\n", cacheobj->nodeid);
        }

        // Forward the request to SILT node
      }else if (strcasecmp(method, "PUT") == 0) {
        cout << "PUT RAW [" << strlen(buf) << "] " << bytes_to_hex(buf) << endl;
        cout << "PUT " << bytes_to_hex(key) << " -> "<< bytes_to_hex(value) << endl;

        char *ret_desc = "2222\n";
        Rio_writen(browserfd, ret_desc, strlen(ret_desc));
        cout << "@@@@@ Server send rc " << ret_desc << endl;
      }else if (strcasecmp(method, "EXIT") == 0){
        printf("EXIT...");
        return;
      }else{
        char *notsupport = "METHOD NOT SUPPORTED\n";
        cout << "EEEEE WTF!" << bytes_to_hex(buf) << endl;
        Rio_writen(browserfd, notsupport, strlen(notsupport));
        Rio_writen(browserfd, buf, strlen(buf));
      }

    }

}


int main(int argc, char **argv){
    int listenfd, browserfd, port, i;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    // struct hostent *hp;
    // char *haddrp;
    pthread_t tid[THREAD_POOL_SIZE];

    /* Initialize sbuf & cache */
    sbuf_init(&sbuf, SBUFSIZE);
    cache_init();

  // if (argc != 1) {
  //  fprintf(stderr, "usage: %s <port>\n", argv[0]);
  //  exit(0);
  // }
//    printmf("%s%s%s", msg_user_agent, msg_accept, msg_accept_encoding);
  port = (argc != 2)?PORT:atoi(argv[1]);
    printf("Running proxy at port %d..\n", port);
  
    // printf("Pre-forking %d working threads..", THREAD_POOL_SIZE);
    /* Prefork thread to the thread pool */
    for (i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&tid[i], NULL, request_handler, NULL);
    }
    // printf("done!\n");

    /* Listen to incoming clients.. forever */
    if ((listenfd = open_listenfd(port)) < 0) {
        fprintf(stderr, "Open_listenfd error: %s\n", strerror(errno));
        exit(0);
    }
    
    printf("Listening on port %d\n", port);
    while (1) {
        clientlen = sizeof(clientaddr);
        if ((browserfd = accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0)
          exit(1);
        printf("Accepted browserfd = %d to s_buf\n", browserfd);
        sbuf_insert(&sbuf, browserfd); /* Insert browserfd in buffer */

    }

    sbuf_deinit(&sbuf);
    
    return 0;
}
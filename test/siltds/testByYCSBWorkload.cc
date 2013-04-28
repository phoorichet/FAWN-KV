/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>

#include "fawnds_factory.h"
#include "rate_limiter.h"
#include "global_limits.h"
#include "datastat.h"
#include "print.h"
#include "csapp.h"

#include "preprocessTrace.h"

using namespace std;
using namespace tbb;
using namespace silt;

#define MAX_QUERIES 1048576 

int32_t num_threads = 1;
int64_t max_ops_per_sec = 1000000000L;
int64_t convert_rate = 1000000000L;
int64_t merge_rate = 1000000000L;
double successful_get_ratio = 1.;

FawnDS *h;
size_t val_len;

RateLimiter *rate_limiter;

struct Query q_buf[MAX_QUERIES];
uint64_t num_query_sent = 0;
uint64_t num_query_read = 0;
bool done;

pthread_mutex_t query_lock;
pthread_mutex_t stat_lock;

DataStat *latency_put;
DataStat *latency_get;

char *master_ip;
int master_port;

#define MAXLINE  8096


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



/******************************** 
 * Client/server helper functions
 ********************************/
/*
 * open_clientfd - open connection to server at <hostname, port> 
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error. 
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, int port) 
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
  return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
    (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
  return -1;
    return clientfd;
}
/* $end open_clientfd */

/*  
 * open_listenfd - open and return a listening socket on port
 *     Returns -1 and sets errno on Unix error.
 */
/* $begin open_listenfd */
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
/* $end open_listenfd */

/******************************************
 * Wrappers for the client/server helper routines 
 ******************************************/
int Open_clientfd(char *hostname, int port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) {
  if (rc == -1)
      unix_error("Open_clientfd Unix error");
  else        
      dns_error("Open_clientfd DNS error");
    }
    return rc;
}

int Open_listenfd(int port) 
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
  unix_error("Open_listenfd error");
    return rc;
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

/***************************************************
 * Wrappers for dynamic storage allocation functions
 ***************************************************/

void *Malloc(size_t size) 
{
    void *p;

    if ((p  = malloc(size)) == NULL)
  unix_error("Malloc error");
    return p;
}

void *Realloc(void *ptr, size_t size) 
{
    void *p;

    if ((p  = realloc(ptr, size)) == NULL)
  unix_error("Realloc error");
    return p;
}

void *Calloc(size_t nmemb, size_t size) 
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
  unix_error("Calloc error");
    return p;
}

void Free(void *ptr) 
{
    free(ptr);
}


/******************************************
 * Wrappers for the Standard I/O functions.
 ******************************************/
void Fclose(FILE *fp) 
{
    if (fclose(fp) != 0)
  unix_error("Fclose error");
}

FILE *Fdopen(int fd, const char *type) 
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
  unix_error("Fdopen error");

    return fp;
}

char *Fgets(char *ptr, int n, FILE *stream) 
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
  app_error("Fgets error");

    return rptr;
}

FILE *Fopen(const char *filename, const char *mode) 
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
  unix_error("Fopen error");

    return fp;
}

void Fputs(const char *ptr, FILE *stream) 
{
    if (fputs(ptr, stream) == EOF)
  unix_error("Fputs error");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream)) 
  unix_error("Fread error");
    return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
  unix_error("Fwrite error");
}


void Close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
  unix_error("Close error");
}


/* 
==========================================================================
*/
int send_put(int clientfd, rio_t *rio, char *key, char* value){
    char buf_str[MAXLINE], rc[MAXLINE], rc_buf[MAXLINE];
    char *buf_ptr = buf_str;
    const char *put_method = "PUT ";
    const char *space = " ";
    const char *rn = "\n";
    strncpy(buf_ptr, put_method, strlen(put_method));
    string test;
    buf_ptr += strlen(put_method);
    
    strncpy(buf_ptr, key, strlen(key));
    buf_ptr += strlen(key);
    
    strncpy(buf_ptr, space, strlen(space));
    buf_ptr += strlen(space);

    strncpy(buf_ptr, value, strlen(value));
    buf_ptr += strlen(value);

    strncpy(buf_ptr, rn, strlen(rn));
    buf_ptr += strlen(rn);

    Rio_writen(clientfd, buf_str, strlen(buf_str));
    cout << "&&&&& PUT Sent " << bytes_to_hex(key) << "->" << bytes_to_hex(value) << endl;
    cin >> test;
    Rio_readlineb(rio, rc_buf, MAXLINE);
    sscanf(rc_buf, "%s", rc);
    cout << "$$$$$ PUT rc = " << rc_buf << endl;

    // cout << "$$$$$$ " << bytes_to_hex(buf_str) << endl;
    // cout << "size: " << strlen(buf_str) << endl;
    return 0;
}

int send_get(int clientfd, rio_t *rio, char *key, char* value){
    char buf_str[MAXLINE], rc[MAXLINE], rc_buf[MAXLINE];
    char *buf_ptr = buf_str;
    const char *put_method = "GET ";
    const char *space = " ";
    const char *rn = "\n";
    strncpy(buf_ptr, put_method, strlen(put_method));
    buf_ptr += strlen(put_method);
    
    strncpy(buf_ptr, key, strlen(key));
    buf_ptr += strlen(key);

    strncpy(buf_ptr, rn, strlen(rn));
    buf_ptr += strlen(rn);

    Rio_writen(clientfd, buf_str, strlen(buf_str));
    Rio_readlineb(rio, rc_buf, MAXLINE);
    sscanf(rc_buf, "%s", rc);
    cout << "$$$$$ GET rc = " << rc << endl;

    // cout << "$$$$$$ " << bytes_to_hex(buf_str) << endl;
    // cout << "size: " << strlen(buf_str) << endl;
    return 0;
}

void *query_reader(void *p) {
    FILE *fp = (FILE *) p;
    size_t n; 
                
    printf("starting thread: query_reader!\n");                

    if (fread(&val_len, sizeof(size_t), 1, fp))
        cout << "val_len=" << val_len << "bytes" <<  endl;
    else
        exit(-1);

    while(!feof((FILE*) fp)) {
        if (num_query_read < num_query_sent + MAX_QUERIES/2) {
            n = fread(&(q_buf[num_query_read % MAX_QUERIES]), sizeof(Query), 1024,  fp);
             
            num_query_read = num_query_read + n;
            
            //printf("[query_reader] num_query_read=%ld\n", num_query_read);
        } else {
            //cout << "query_reader is sleeping! num_query_read=" << num_query_read << " num_query_sent=" << num_query_sent << endl;
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 100 * 1000;
            nanosleep(&ts, NULL);
        }
    }
    done = true;
    fclose(fp);
    printf("killing thread: query_reader!\n");                
    pthread_exit(NULL);
}

void *query_sender(void * id) {
    struct Query q;
    FawnDS_Return ret;
    long t = (long) id;
    char val[(MAXLEN + sizeof(uint64_t) - 1) / sizeof(uint64_t) * sizeof(uint64_t)];
    unsigned int data_seed = 0;
    struct timespec ts;
    double last_time_, current_time_, latency;
    printf("starting thread: query_sender%ld!\n", t);

    // Connect to the master
    int clientfd, port;
    // char *host, buf[MAXLINE], *xmlconfig, rc[MAXLINE];
    // ssize_t n;
    rio_t rio;
    // char method[MAXLINE], key[MAXLINE], value[MAXLINE];
    int rc;
    clientfd = 4;
    clientfd = Open_clientfd(master_ip, master_port);
    Rio_readinitb(&rio, clientfd);
    cout << "##### client connected to " << master_ip << ":" << master_port << endl;
    char* chr = "A";
    while (1) {

        pthread_mutex_lock(&query_lock);

        if (num_query_sent < num_query_read) {
            uint64_t cur = num_query_sent;
            num_query_sent ++;
            q = q_buf[cur % MAX_QUERIES];   // copy data to ensure no stall access (with some concurrency penalty)

            pthread_mutex_unlock(&query_lock);

            rate_limiter->remove_tokens(1);
            if (q.tp == PPUT) {
                uint64_t r = (static_cast<uint64_t>(rand_r(&data_seed)) << 32) | static_cast<uint64_t>(rand_r(&data_seed));
                uint64_t* p = reinterpret_cast<uint64_t*>(val);
                const uint64_t* p_end = p + (val_len / sizeof(uint64_t));
                while (p != p_end)
                    *p++ = r++;

                clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
                last_time_ = static_cast<int64_t>(ts.tv_sec) * 1000000000Lu + static_cast<int64_t>(ts.tv_nsec);
                

                // ret = h->Put(ConstRefValue(q.hashed_key, 20), 
                //              ConstRefValue(val, val_len));         

                rc = send_put(clientfd, &rio, q.hashed_key, chr);
                if (rc != 0) {
                    printf("error! h->Put() return value=%d, expected=%d, operation%llu\n", 
                           rc, OK, static_cast<unsigned long long>(cur));
                    //exit(1);
                }
                clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
                current_time_ = static_cast<int64_t>(ts.tv_sec) * 1000000000Lu + static_cast<int64_t>(ts.tv_nsec);
                latency = current_time_ - last_time_;

                pthread_mutex_lock(&stat_lock);
                latency_put->insert(latency);
                pthread_mutex_unlock(&stat_lock);



            } else if (q.tp == GET) {
                FawnDS_Return expected_ret;
                if (static_cast<double>(rand()) / static_cast<double>(RAND_MAX) <= successful_get_ratio)
                    expected_ret = OK;
                else {
                    // make the query to be unsuccessful assuming extremely low key collision
                    for (size_t i = 0; i < 20; i += 2)
                        *reinterpret_cast<uint16_t*>(q.hashed_key + i) = static_cast<uint16_t>(rand());
                    expected_ret = KEY_NOT_FOUND;
                }

                SizedValue<MAXLEN> read_data;
                clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
                last_time_ = static_cast<int64_t>(ts.tv_sec) * 1000000000Lu + static_cast<int64_t>(ts.tv_nsec);                
                
                char rv[MAXLINE];
                rc = send_put(clientfd, &rio, q.hashed_key, rv);
                if (rc != 0) {
                    printf("error! h->GET() return value=%d, expected=%d, operation%llu\n", 
                           rc, OK, static_cast<unsigned long long>(cur));
                    //exit(1);
                }


                // ret = h->Get(ConstRefValue(q.hashed_key, 20), read_data);
                // if (ret != expected_ret) {
                //     printf("error! h->Get() return value=%d, expected=%d, operation%llu\n", 
                //            ret, expected_ret, static_cast<unsigned long long>(cur));
                //     //exit(1);
                // }

                clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
                current_time_ = static_cast<int64_t>(ts.tv_sec) * 1000000000Lu + static_cast<int64_t>(ts.tv_nsec);                
                latency = current_time_ - last_time_;

                pthread_mutex_lock(&stat_lock);
                latency_get->insert(latency);
                pthread_mutex_unlock(&stat_lock);

            } else {
                printf("unknown query type %d.\n", q.tp);
            }

        } else {
            pthread_mutex_unlock(&query_lock);

            if (done) {
                break;
            }
            else {
                //cout << "query_sender is sleeping! num_query_read=" << num_query_read << " num_query_sent=" << num_query_sent << endl;
                struct timespec ts;
                ts.tv_sec = 0;
                ts.tv_nsec = 100 * 1000;
                nanosleep(&ts, NULL);
            }
        }
    } /* end of while (done) */
    printf("killing thread: query_sender%ld!\n", t);
    pthread_exit(NULL);
} /* end of query_sender */


void replay(string recfile) {
    pthread_t reader_thread;
    pthread_t *sender_threads = new pthread_t[num_threads];

    // see if the recfile is good
    FILE *fp = fopen(recfile.c_str(), "r");
    if (fp == NULL) {
        cout << "can not open file " << recfile << endl;
        exit(-1);
    }

    pthread_mutex_init(&query_lock, 0 );
    pthread_mutex_init(&stat_lock, 0 );
    num_query_read = num_query_sent = 0;
    done = false;

    latency_put = new DataStat("PUT-LATENCY(ns)", 0, 100000000, 10000., 10., 1.02);
    latency_get = new DataStat("GET-LATENCY(ns)", 0, 100000000, 10000., 10., 1.02);

    // create threads!
    pthread_create(&reader_thread, NULL, query_reader, (void *) fp);
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000 * 1000;
    nanosleep(&ts, NULL);
    for (int t = 0; t < num_threads; t++) {
        pthread_create(&sender_threads[t], NULL, query_sender, (void *) t);
    }

    // wait for threads 
    pthread_join(reader_thread, NULL);
    for (int t = 0; t < num_threads; t++) {
        pthread_join(sender_threads[t], NULL);
    }

    // output and destroy everything
    printf("total %llu ops in %f sec: tput = %.2f KQPS\n", 
           static_cast<unsigned long long> (latency_put->num() + latency_get->num()), 
           0.000000001 * (latency_put->sum() + latency_get->sum()),
           1000000. * (latency_put->num() + latency_get->num()) / (latency_put->sum() + latency_get->sum())        );

    latency_put->summary();
    latency_get->summary();

    if (latency_put->num()) {
        latency_put->cdf(1.02, true);
    }

    if (latency_get->num()) {
        latency_get->cdf(1.02, true);
    }

    pthread_mutex_destroy(&query_lock);
    pthread_mutex_destroy(&stat_lock);
    delete latency_put;
    delete latency_get;

    h->Flush();
    sleep(10);  // these sleep() gives time for FawnDS_Monitor to print status messages after the store reaches a steady state
    sync();
    sleep(5);
}


void usage() {
    cout << "usage: testByYCSBWorkload conf_file load_workload_name trans_workload_name master_ip master_port [-t num_threads] [-r max_ops_per_sec] [-c convert_rate] [-m merge_rate] [-s successful_get_ratio]" << endl 
         << "e.g. ./testByYCSBWorkload testConfigs/bdb.xml testWorkloads/update_only" << endl;

}

int main(int argc, char **argv) {

    char ch;
    while ((ch = getopt(argc, argv, "t:r:c:m:s:")) != -1)
        switch (ch) {
        case 't': num_threads = atoi(optarg); break;
        case 'r': max_ops_per_sec = atoll(optarg); break;
        case 'c': convert_rate = atoll(optarg); break;
        case 'm': merge_rate = atoll(optarg); break;
        case 's': successful_get_ratio = atof(optarg); break;
        default:
            usage();
            exit(-1);
        }

    argc -= optind;
    argv += optind;

    if (argc < 5) {
        usage();
        exit(-1);
    }

    cout << "num_threads: " << num_threads << endl;
    cout << "max_ops_per_sec: " << max_ops_per_sec << endl;
    cout << "convert_rate: " << convert_rate << endl;
    cout << "merge_rate: " << merge_rate << endl;
    cout << "successful_get_ratio: " << successful_get_ratio << endl;

    string conf_file = string(argv[0]);
    string load_workload = string(argv[1]);
    string trans_workload = string(argv[2]);

    master_ip = argv[3];
    master_port = atoi(argv[4]);

    // clean dirty pages to reduce anomalies
    sync();

    GlobalLimits::instance().set_convert_rate(convert_rate);
    GlobalLimits::instance().set_merge_rate(merge_rate);

    rate_limiter = new RateLimiter(0, 1000000000L, 1, 1);   // virtually unlimted
    GlobalLimits::instance().disable();

    cout << "process load ... " << endl;
    replay(load_workload + ".load");
    cout << "process load ... done" << endl;
    cout << endl << endl << endl;

    delete rate_limiter;

    rate_limiter = new RateLimiter(0, max_ops_per_sec, 1, 1000000000L / max_ops_per_sec);    
    GlobalLimits::instance().enable();

    cout << "process transaction ... " << endl;
    replay(trans_workload + ".trans");
    cout << "process transaction ... done" << endl;

    delete rate_limiter;

    h->Close();
    delete h;
}

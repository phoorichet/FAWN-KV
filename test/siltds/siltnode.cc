#include <iostream>
#include <stdio.h>
#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <assert.h>

#include "fawnds_factory.h"
#include "fawnds_types.h"
#include "fawnds.h"
#include "csapp.h"
#include "hashutil.h"
#include "print.h"

using namespace std;
using namespace fawn;
using namespace silt;

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

int init_silt(){
   return 0;
}


int main(int argc, char **argv){
    int clientfd, port;
    char *host, buf[MAXLINE], *xmlconfig, rc[MAXLINE];
    ssize_t n;
    rio_t rio;
    char method[MAXLINE], key[MAXLINE], value[MAXLINE];

    if(argc!=4){
      fprintf(stderr, "usage: %s <host> <port> <xml config>\n", argv[0]); exit(0);
    }
    host = argv[1];
    port = atoi(argv[2]);
    xmlconfig = argv[3];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    // request connection
    char *conn_str = " conn 128.131.2.18\r\n";
    Rio_writen(clientfd, conn_str, strlen(conn_str));
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s", rc);
    printf("+++++++> client request result: %s\n", rc);

    // Init silt
    cout << "+++++ Using XML file: " << xmlconfig << endl;
    silt::FawnDS *h = silt::FawnDS_Factory::New(xmlconfig); // test_num_records_
    if (h->Create() != silt::OK) {
        cout << "cannot create FAWNDS!" << endl;
        exit(0);
    }

    Value ret_data;
    ret_data.resize(0);

    while( (n = Rio_readlineb(&rio, buf, MAXLINE)) > 0 ){
      printf("[SILT NODE] >>>>> %s\n", buf);
      sscanf(buf, "%s %s %s", method, key, value);
      if (strcasecmp(method, "GET") == 0){
        FawnDS_Return ret = h->Get(ConstRefValue(key, strlen(key)), ret_data);
        if (ret != silt::OK) {
            printf("error! h->Get() return key=%s, ret=%d\n", 
                   key, ret);
            //exit(1);
        } 
      }else if (strcasecmp(method, "PUT") == 0){
         FawnDS_Return ret = h->Put(ConstRefValue(key, strlen(key)), ConstRefValue(value, strlen(value)));
         if (ret != silt::OK) {
          printf("error! h->PUT() return key=%s, value=%s, ret=%d\n", 
                 key, value, ret);
          //exit(1);
        } 
      }

    }
    

    h->Close();
    delete h;


    Close(clientfd);
    exit(0);
    
    return 0;
}
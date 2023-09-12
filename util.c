// Import libraries
#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define BACKLOGLENGTH 5

int master_fd = -1;
pthread_mutex_t accept_con_mutex = PTHREAD_MUTEX_INITIALIZER;



// Begins server on port
void init(int port) {
  int sd;
  struct sockaddr_in addr;
  int ret_val;
  int flag;

  // create socket & set for re-use
  if( (sd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    perror("socket failed");
    exit(1);
  }
  flag = 1;
  if( (ret_val = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int))) != 0) {
    perror("setsockopt failed");
    exit(1);
  }
  
  // initialize sockaddr fields & bind
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if( (ret_val = bind(sd, (struct sockaddr*) &addr, sizeof(addr))) != 0) {
    perror("bind failed");
    exit(1);
  }

  // listen on port, set master_fd for server, and print
  if( (ret_val = listen(sd, BACKLOGLENGTH)) != 0) {
    perror("listen failed");
    exit(1);
  }
  master_fd = sd;
  printf("UTILS.O: Server Started on Port %d\n",port);
}



// Accepts a new connection by safely calling accept()
int accept_connection(void) {
  int newsock;
  struct sockaddr_in new_recv_addr;
  uint addr_len;
  addr_len = sizeof(new_recv_addr);
  
  // lock
  if(pthread_mutex_lock(&accept_con_mutex) != 0) {
    perror("lock failed in accept_connection\n");
    return -1;
  }
  // accept connection
  if( (newsock = accept(master_fd, (struct sockaddr*)&new_recv_addr, (socklen_t*)&addr_len)) < 0){
    perror("accept failed in accept_connection");
    return -1;
  }
  // unlock
  if(pthread_mutex_unlock(&accept_con_mutex) != 0) {
    perror("unlock failed in accept_connection\n");
    return -1;
  }
  return newsock;
}





// Gets fd from accept_connection, reads in HTTP response
int get_request(int fd, char *filename) {
    
  char buf[2048];
  char* fName;
  int i;

  printf("Getting request: %s\n", filename);
  // read in bytes from fd
  if( (i = read(fd, buf, 2048)) == -1) {
    perror("read in get_request failed");
    return -1;
  }
  char firstLine[2048];
  char c;

  // read first line in, end with nullChar
  for(i=0; i<sizeof(firstLine); i++) {
    c = buf[i];
    if(c == '\n'){
      firstLine[i] = '\0';
      break;
    }
    firstLine[i] = c;
  }
  printf("%s\n", firstLine);
  // get first space-separated item
  char* tokens = strtok(firstLine, " ");
  i=0;

  // checks for invalid first line:
  while(tokens != NULL) {
    if(i==0) {
      if(strcmp(tokens, "GET")!=0) {
        return -1;
      }
    }
    else if(i==1) {
      fName = tokens; // get next space-separated item
    }
    else if(i==2) {
      int l = strlen(tokens); 
      // tokens should be 9 long and the last ascii value should be carriage newline (13)
      if(l != 9 || (int)tokens[l-1] != 13 || ( strstr(tokens, "HTTP/1.0")==NULL && strstr(tokens, "HTTP/1.1")==NULL ) ) {  
        // HTTP not in tokens
        return -1;
      }
    }
    else {
      // first line has too many spaces
      return -1;
    }
    tokens = strtok(NULL, " ");
    i++;
  }
  // check if .. or // in the file's name
  if (strstr(fName, "..") != NULL || strstr(fName, "//") != NULL) {
    return -1;
  }
  // copy result into filename & return
  strncpy(filename, fName, 2048);
  return 0;
}




int return_result(int fd, char *content_type, char *buf, int numbytes) { 
  char response_header[2048]; 
  int bytes_sent; 
  // Construct response header 
  sprintf(response_header, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\nConnection: Close\n\n", content_type, numbytes); 
  // Send response header 
  if((bytes_sent = send(fd, response_header, strlen(response_header), 0)) == -1) {
     perror("send"); 
     return -1; 
  } 
  // Send response body 
  if((bytes_sent = send(fd, buf, numbytes, 0)) == -1) {
    perror("send");
    return -1; 
  } 
  // Close socket 
  close(fd); 
  return 0; 
}


int return_error(int fd, char *buf) { 
  char response_header[2048]; 
  int bytes_sent; 
  // Construct response header 
  sprintf(response_header, "HTTP/1.1 404 Not Found\nContent-Type: text/html\nContent-Length: %ld\nConnection: Close\n\n", strlen(buf)); 
  // Send response header 
  if((bytes_sent = send(fd, response_header, strlen(response_header), 0)) == -1) {
    perror("send"); 
    return -1; 
  } 
  // Send response body 
  if((bytes_sent = send(fd, buf, strlen(buf), 0)) == -1) { 
    perror("send"); 
    return -1; 
  } 
  // Close socket 
  close(fd);
  return 0; 
}

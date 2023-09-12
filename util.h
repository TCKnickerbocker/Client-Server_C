#ifndef _UTIL_H
#define _UTIL_H

/*
 * init - Initialize a connection on the specified port.
 *
 * Parameters:
 *   port: The port number to initialize the connection.
 *
 * This function should be called only once to set up the connection.
 */
void init(int port);

/*
 * accept_connection - Accept a connection and return a file descriptor for further reading.
 *
 * Returns:
 *   A file descriptor for the accepted connection on success, or -1 on error.
 */
int accept_connection(void);

/*
 * get_request - Retrieve a request from a file descriptor and store the filename
 *
 * Parameters:
 *   fd: File descriptor where the request is received.
 *   filename: Buffer to store the requested file's name.
 *
 * Returns:
 *   0 on success, nonzero on failure.
 */
int get_request(int fd, char *filename);

/*
 * return_result - Send file content to the client and clean up the connection
 *
 * Parameters:
 *   fd: File descriptor to send the result to.
 *   content_type: Pointer to the content type (e.g., "text/html").
 *   buf: Pointer to the memory location containing the file data.
 *   numbytes: Number of bytes in the file data.
 *
 * Returns:
 *   0 on success, nonzero on failure.
 */
int return_result(int fd, char *content_type, char *buf, int numbytes);

/*
 * return_error - Send an error message in response to a bad request and clean up the connection
 *
 * Parameters:
 *   fd: File descriptor to send the error to.
 *   buf: Pointer to the location of the error text.
 *
 * Returns:
 *   0 on success, nonzero on failure.
 */
int return_error(int fd, char *buf);


#endif /* _UTIL_H */
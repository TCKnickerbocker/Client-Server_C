# Multithreaded Client-Server File Transfer
## This project is a fully functional multithreaded client and server written in C. The primary purpose of this application is to allow clients to request files from the server, and the server responds by dispatching worker threads to return the requested file contents to the client.

## Features
### Multithreaded Architecture: The server employs multithreading to handle multiple client requests concurrently, improving overall server efficiency.

### File Transfer: Clients can request specific files from the server, and the server retrieves and sends the requested file contents to the client.

### Caching: The server implements a caching mechanism to store recently requested files in a circular buffer. This caching strategy helps minimize response times for frequently requested files.

## Usage
### Server
To run the server, execute the following command:

bash
Copy code
./server <port> <path> <num_dispatcher> <num_workers> <queue_length> <cache_size>
<port>: The port on which the server listens for incoming client requests.
<path>: The server's root directory where files are served from.
<num_dispatcher>: The number of dispatcher threads.
<num_workers>: The number of worker threads.
<queue_length>: The maximum length of the request queue.
<cache_size>: The size of the cache for storing recently requested files.

Or simply type <strong>web_server</strong> and <strong>run</strong> into a terminal

### Client
The client can request files from the server using appropriate requests. This can be done simply via the terminal, if the user would like
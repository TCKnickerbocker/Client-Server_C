CC = gcc
CFLAGS = -D_REENTRANT -g -Wall -Wno-unknown-pragmas
LDFLAGS = -lpthread -pthread

### COMPILATION:
web_server: server.c server.h util.o
	${CC} $(CFLAGS) -o web_server server.c util.o ${LDFLAGS}

run:
	@# Port: passed_in | Path: ./myLogs | num_dispatcher: 5 | num_workers: 5
	@# queue_length: 50 | cache_size: 50
	@chmod +x web_server_sol
	@read -p "Enter a random port between 1024 & 65536: " sock_in; \
	./web_server_sol $$sock_in ./myLogs 100 100 50 50

### UTILITIES:
force_kill:
	@echo -n "Force Killing web_server proc PID: "; ps -C "web_server" -o pid=
	@ps -C "web_server" -o pid= | xargs kill -9
	@echo -n "Force Killing parent proc PID: "; ps -C "make" -o pid=
	@ps -C "make" -o pid= | xargs kill -9

clean:
	rm -f web_server webserver_log *.tar.gz
	@clear
	@echo "Succesful Clean"

####################################################################

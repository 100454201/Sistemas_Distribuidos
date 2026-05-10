CC     = gcc
CFLAGS = -Wall -g -I/usr/include/tirpc

.PHONY: all clean rpc

all: rpc server log_server

rpc: log_rpc.h

log_rpc.h log_rpc_clnt.c log_rpc_svc.c log_rpc_xdr.c: log_rpc.x
	rpcgen log_rpc.x

# Compilar log_rpc_xdr.o suprimiendo warnings del codigo generado por rpcgen
log_rpc_xdr.o: log_rpc_xdr.c log_rpc.h
	$(CC) $(CFLAGS) -Wno-unused-variable -c log_rpc_xdr.c -o log_rpc_xdr.o

server: server.c log_rpc_clnt.c log_rpc_xdr.o log_rpc.h
	$(CC) $(CFLAGS) -o server server.c log_rpc_clnt.c log_rpc_xdr.o -lpthread -ltirpc

log_server: log_rpc_server_impl.c log_rpc_svc.c log_rpc_xdr.o log_rpc.h
	$(CC) $(CFLAGS) -o log_server log_rpc_server_impl.c log_rpc_svc.c log_rpc_xdr.o -ltirpc

clean:
	rm -f server log_server log_rpc.h log_rpc_clnt.c log_rpc_svc.c log_rpc_xdr.c log_rpc_xdr.o *.o
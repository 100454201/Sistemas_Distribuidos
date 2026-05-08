CC     = gcc
CFLAGS = -Wall -g

.PHONY: all clean rpc

# Genera los stubs RPC y compila ambos ejecutables
all: rpc server log_server

# Genera los ficheros RPC a partir de la interfaz .x
rpc: log_rpc.h

log_rpc.h log_rpc_clnt.c log_rpc_svc.c log_rpc_xdr.c: log_rpc.x
	rpcgen log_rpc.x

# Servidor de mensajeria (actua como cliente RPC)
server: server.c log_rpc_clnt.c log_rpc_xdr.c log_rpc.h
	$(CC) $(CFLAGS) -o server server.c log_rpc_clnt.c log_rpc_xdr.c -lpthread

# Servidor ONC-RPC de log de operaciones
log_server: log_rpc_server_impl.c log_rpc_svc.c log_rpc_xdr.c log_rpc.h
	$(CC) $(CFLAGS) -o log_server log_rpc_server_impl.c log_rpc_svc.c log_rpc_xdr.c

clean:
	rm -f server log_server log_rpc.h log_rpc_clnt.c log_rpc_svc.c log_rpc_xdr.c *.o

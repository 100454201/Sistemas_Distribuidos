# Makefile - Ejercicio Evaluable 3 - Sistemas Distribuidos
# Servicio de tuplas con ONC RPC

CC      = gcc
CFLAGS  = -Wall -Wextra -g -fPIC -I/usr/include/tirpc
LDFLAGS = -lpthread -ltirpc

RPCGEN_AUTO = clavesRPC.h clavesRPC_clnt.c clavesRPC_svc.c \
              clavesRPC_xdr.c clavesRPC_client.c

SERVER_OBJS  = clavesRPC_server.o clavesRPC_svc.o clavesRPC_xdr.o claves.o
PROXY_OBJS   = proxy-rpc.o clavesRPC_clnt.o clavesRPC_xdr.o

LIB_CLAVES       = libclaves.so
LIB_PROXY_CLAVES = libproxyclaves.so

SERVIDOR        = clavesRPC_server
APP_CLIENTE     = app-cliente
APP_CLIENTE_RPC = app-cliente-rpc

.PHONY: all clean rpcgen local distribuido help

# Objetivo principal
all: local distribuido

# ===========================
# PARTE A: Version no distribuida
# ===========================
local: $(LIB_CLAVES) $(APP_CLIENTE)

claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c claves.c -o claves.o

$(LIB_CLAVES): claves.o
	$(CC) -shared -o $(LIB_CLAVES) claves.o $(LDFLAGS)

$(APP_CLIENTE): app-cliente.c $(LIB_CLAVES)
	$(CC) $(CFLAGS) app-cliente.c -o $(APP_CLIENTE) \
		-L. -lclaves $(LDFLAGS) -Wl,-rpath,.

# ===========================
# PARTE B: Version distribuida con RPC
# ===========================
distribuido: $(LIB_PROXY_CLAVES) $(SERVIDOR) $(APP_CLIENTE_RPC)

clavesRPC_server.o: clavesRPC_server.c clavesRPC.h claves.h
	$(CC) $(CFLAGS) -c clavesRPC_server.c -o clavesRPC_server.o

clavesRPC_svc.o: clavesRPC_svc.c clavesRPC.h
	$(CC) $(CFLAGS) -c clavesRPC_svc.c -o clavesRPC_svc.o

clavesRPC_xdr.o: clavesRPC_xdr.c clavesRPC.h
	$(CC) $(CFLAGS) -c clavesRPC_xdr.c -o clavesRPC_xdr.o

clavesRPC_clnt.o: clavesRPC_clnt.c clavesRPC.h
	$(CC) $(CFLAGS) -c clavesRPC_clnt.c -o clavesRPC_clnt.o

$(SERVIDOR): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVIDOR) \
		-L. -lclaves $(LDFLAGS) -Wl,-rpath,.

proxy-rpc.o: proxy-rpc.c clavesRPC.h claves.h
	$(CC) $(CFLAGS) -c proxy-rpc.c -o proxy-rpc.o

$(LIB_PROXY_CLAVES): $(PROXY_OBJS)
	$(CC) -shared -o $(LIB_PROXY_CLAVES) $(PROXY_OBJS) $(LDFLAGS)

$(APP_CLIENTE_RPC): app-cliente.c $(LIB_PROXY_CLAVES)
	$(CC) $(CFLAGS) app-cliente.c -o $(APP_CLIENTE_RPC) \
		-L. -lproxyclaves $(LDFLAGS) -Wl,-rpath,.

# Regla para invocar rpcgen (preserva clavesRPC_server.c)
rpcgen:
	@echo "[rpcgen] Generando archivos desde clavesRPC.x ..."
	@[ -f clavesRPC_server.c ] && mv clavesRPC_server.c /tmp/clavesRPC_server.c.bak || true
	@rm -f Makefile.clavesRPC clavesRPC_client.c
	rpcgen -aNM clavesRPC.x
	@[ -f /tmp/clavesRPC_server.c.bak ] && mv /tmp/clavesRPC_server.c.bak clavesRPC_server.c || true
	@echo "[rpcgen] Hecho."

# ===========================
# Limpieza (preserva clavesRPC_server.c)
# ===========================
clean:
	rm -f *.o *.so $(APP_CLIENTE) $(APP_CLIENTE_RPC) $(SERVIDOR)
	rm -f $(RPCGEN_AUTO)
	@echo "Limpieza completada. clavesRPC_server.c preservado."

help:
	@echo "Uso:"
	@echo "  1. Primera vez: make rpcgen && make all"
	@echo "  2. Sucesivas:   make all"
	@echo "  3. Limpiar:     make clean"
	@echo ""
	@echo "Ejecucion:"
	@echo "  Terminal 1: ./$(SERVIDOR)"
	@echo "  Terminal 2: export IP_TUPLAS=localhost && ./$(APP_CLIENTE_RPC)"
# Makefile - Ejercicio Evaluable 2 - Sistemas Distribuidos
# Compilacion de version no distribuida y distribuida con sockets TCP

CC     = gcc
CFLAGS = -Wall -Wextra -g -fPIC
LDFLAGS = -pthread -lrt

# Archivos objeto
CLAVES_OBJ    = claves.o
PROXY_OBJ     = proxy-sock.o

# Bibliotecas dinamicas
LIB_CLAVES       = libclaves.so
LIB_PROXY_CLAVES = libproxyclaves.so

# Ejecutables
APP_CLIENTE      = app-cliente
APP_CLIENTE_SOCK = app-cliente-sock
SERVIDOR_SOCK    = servidor_sock

.PHONY: all clean local distribuido help

# Objetivo principal
all: local distribuido

# Version no distribuida
local: $(LIB_CLAVES) $(APP_CLIENTE)

# Version distribuida con sockets TCP
distribuido: $(LIB_PROXY_CLAVES) $(SERVIDOR_SOCK) $(APP_CLIENTE_SOCK)

# ===========================
# PARTE A: Version no distribuida
# ===========================

claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c claves.c -o claves.o

$(LIB_CLAVES): $(CLAVES_OBJ)
	$(CC) -shared -o $(LIB_CLAVES) $(CLAVES_OBJ) $(LDFLAGS)

$(APP_CLIENTE): app-cliente.c $(LIB_CLAVES)
	$(CC) $(CFLAGS) app-cliente.c -o $(APP_CLIENTE) -L. -lclaves $(LDFLAGS) -Wl,-rpath,.

# ===========================
# PARTE B: Version distribuida con sockets TCP
# ===========================

proxy-sock.o: proxy-sock.c claves.h
	$(CC) $(CFLAGS) -c proxy-sock.c -o proxy-sock.o

$(LIB_PROXY_CLAVES): $(PROXY_OBJ)
	$(CC) -shared -o $(LIB_PROXY_CLAVES) $(PROXY_OBJ) $(LDFLAGS)

$(SERVIDOR_SOCK): servidor-sock.c $(LIB_CLAVES) claves.h
	$(CC) $(CFLAGS) servidor-sock.c -o $(SERVIDOR_SOCK) -L. -lclaves $(LDFLAGS) -Wl,-rpath,.

$(APP_CLIENTE_SOCK): app-cliente.c $(LIB_PROXY_CLAVES)
	$(CC) $(CFLAGS) app-cliente.c -o $(APP_CLIENTE_SOCK) -L. -lproxyclaves $(LDFLAGS) -Wl,-rpath,.

# ===========================
# Limpieza
# ===========================

clean:
	rm -f *.o *.so $(APP_CLIENTE) $(APP_CLIENTE_SOCK) $(SERVIDOR_SOCK)
	@echo "Limpieza completada."

# ===========================
# Ayuda
# ===========================

help:
	@echo "Objetivos disponibles:"
	@echo "  make all          - Compilar ambas versiones"
	@echo "  make local        - Compilar solo version no distribuida"
	@echo "  make distribuido  - Compilar solo version distribuida"
	@echo "  make clean        - Limpiar archivos generados"
	@echo ""
	@echo "Ejecutables generados:"
	@echo "  $(APP_CLIENTE)       - Cliente version local"
	@echo "  $(SERVIDOR_SOCK)     - Servidor version distribuida (TCP)"
	@echo "  $(APP_CLIENTE_SOCK)  - Cliente version distribuida (TCP)"
	@echo ""
	@echo "Uso version distribuida:"
	@echo "  Terminal 1: ./$(SERVIDOR_SOCK) <PUERTO>"
	@echo "  Terminal 2: export IP_TUPLAS=localhost PORT_TUPLAS=<PUERTO>"
	@echo "              ./$(APP_CLIENTE_SOCK)"
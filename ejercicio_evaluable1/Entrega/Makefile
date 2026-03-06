# Makefile - Ejercicio Evaluable 1 - Sistemas Distribuidos
# Compilación de versión no distribuida y distribuida

CC = gcc
CFLAGS = -Wall -Wextra -g -fPIC
LDFLAGS = -pthread -lrt

# Archivos objeto
CLAVES_OBJ = claves.o
PROXY_OBJ = proxy-mq.o

# Bibliotecas dinámicas
LIB_CLAVES = libclaves.so
LIB_PROXY_CLAVES = libproxyclaves.so

# Ejecutables
APP_CLIENTE = app-cliente
APP_CLIENTE_MQ = app-cliente-mq
SERVIDOR_MQ = servidor_mq

.PHONY: all clean local distribuido

# Objetivo principal: compilar ambas versiones
all: local distribuido

# Versión no distribuida (local)
local: $(LIB_CLAVES) $(APP_CLIENTE)

# Versión distribuida (colas de mensajes)
distribuido: $(LIB_PROXY_CLAVES) $(SERVIDOR_MQ) $(APP_CLIENTE_MQ)

# ===========================
# PARTE A: Versión no distribuida
# ===========================

# Compilar objeto de claves.c
claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c claves.c -o claves.o

# Crear biblioteca dinámica libclaves.so
$(LIB_CLAVES): $(CLAVES_OBJ)
	$(CC) -shared -o $(LIB_CLAVES) $(CLAVES_OBJ) $(LDFLAGS)

# Crear ejecutable del cliente (versión no distribuida)
$(APP_CLIENTE): app-cliente.c $(LIB_CLAVES)
	$(CC) $(CFLAGS) app-cliente.c -o $(APP_CLIENTE) -L. -lclaves $(LDFLAGS) -Wl,-rpath,.

# ===========================
# PARTE B: Versión distribuida con colas de mensajes
# ===========================

# Compilar objeto de proxy-mq.c
proxy-mq.o: proxy-mq.c claves.h protocolo.h
	$(CC) $(CFLAGS) -c proxy-mq.c -o proxy-mq.o

# Crear biblioteca dinámica libproxyclaves.so
$(LIB_PROXY_CLAVES): $(PROXY_OBJ)
	$(CC) -shared -o $(LIB_PROXY_CLAVES) $(PROXY_OBJ) $(LDFLAGS)

# Crear ejecutable del servidor
$(SERVIDOR_MQ): servidor-mq.c $(LIB_CLAVES) claves.h protocolo.h
	$(CC) $(CFLAGS) servidor-mq.c -o $(SERVIDOR_MQ) -L. -lclaves $(LDFLAGS) -Wl,-rpath,.

# Crear ejecutable del cliente (versión distribuida)
$(APP_CLIENTE_MQ): app-cliente.c $(LIB_PROXY_CLAVES)
	$(CC) $(CFLAGS) app-cliente.c -o $(APP_CLIENTE_MQ) -L. -lproxyclaves $(LDFLAGS) -Wl,-rpath,.

# Limpiar archivos generados
clean:
	rm -f *.o *.so $(APP_CLIENTE) $(APP_CLIENTE_MQ) $(SERVIDOR_MQ)
	@echo "Limpieza completada. No olvides eliminar colas si es necesario:"
	@echo "  rm -rf /dev/mqueue/cola_*"

# Ayuda
help:
	@echo "Objetivos disponibles:"
	@echo "  make all         - Compilar ambas versiones (local + distribuida)"
	@echo "  make local       - Compilar solo versión no distribuida"
	@echo "  make distribuido - Compilar solo versión distribuida"
	@echo "  make clean       - Limpiar archivos generados"
	@echo "  make help        - Mostrar esta ayuda"
	@echo ""
	@echo "Ejecutables generados:"
	@echo "  $(APP_CLIENTE)     - Cliente versión local"
	@echo "  $(SERVIDOR_MQ)     - Servidor versión distribuida"
	@echo "  $(APP_CLIENTE_MQ)  - Cliente versión distribuida"
	@echo ""
	@echo "Uso versión distribuida:"
	@echo "  1. Terminal 1: ./$(SERVIDOR_MQ)"
	@echo "  2. Terminal 2: ./$(APP_CLIENTE_MQ)"

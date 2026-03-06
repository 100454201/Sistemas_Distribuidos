/*
 * protocolo.h - Definición del protocolo de comunicación
 * Sistemas Distribuidos - Ejercicio Evaluable 1
 * 
 * Define las estructuras de mensajes para la comunicación
 * cliente-servidor mediante colas de mensajes POSIX.
 */

#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include "claves.h"

/* Tamaños máximos */
#define MAX_KEY_LENGTH 256
#define MAX_VALUE1_LENGTH 256
#define MAX_N_VALUE2 32

/* Nombre de las colas de mensajes */
#define COLA_SERVIDOR "/cola_servidor_sd"
#define COLA_CLIENTE_PREFIX "/cola_cliente_sd_"

/* Tipos de operaciones */
typedef enum {
    OP_DESTROY = 1,
    OP_SET_VALUE,
    OP_GET_VALUE,
    OP_MODIFY_VALUE,
    OP_DELETE_KEY,
    OP_EXIST,
    OP_TERMINAR  /* Para terminar el servidor limpiamente */
} TipoOperacion;

/* Estructura de petición del cliente al servidor */
typedef struct {
    long tipo_mensaje;  /* Tipo de mensaje (para mq_receive) - siempre 1 */
    TipoOperacion operacion;
    pid_t pid_cliente;  /* PID del cliente para identificar cola de respuesta */
    
    /* Parámetros de la operación */
    char key[MAX_KEY_LENGTH];
    char value1[MAX_VALUE1_LENGTH];
    int N_value2;
    float V_value2[MAX_N_VALUE2];
    struct Paquete value3;
} Peticion;

/* Estructura de respuesta del servidor al cliente */
typedef struct {
    long tipo_mensaje;  /* Tipo de mensaje - siempre 1 */
    int resultado;      /* Código de retorno: 0 (éxito), -1 (error servicio), -2 (error comunicación) */
    
    /* Datos de respuesta (para get_value) */
    char value1[MAX_VALUE1_LENGTH];
    int N_value2;
    float V_value2[MAX_N_VALUE2];
    struct Paquete value3;
} Respuesta;

#endif

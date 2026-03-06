/*
 * proxy-mq.c - Proxy del lado del cliente (implementación de la API)
 * Sistemas Distribuidos - Ejercicio Evaluable 1
 * 
 * Implementa las funciones de la API del servicio de tuplas,
 * pero comunicándose con el servidor mediante colas de mensajes POSIX.
 * 
 * Este código se compila en libproxyclaves.so para ser usado por los clientes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>
#include "claves.h"
#include "protocolo.h"

/* Cola del servidor (compartida por todos los clientes) */
static mqd_t cola_servidor = -1;

/* Cola del cliente (única por proceso) */
static mqd_t cola_cliente = -1;
static char nombre_cola_cliente[64] = {0};

/**
 * Función auxiliar: Inicializa la comunicación con el servidor
 * Abre la cola del servidor y crea la cola del cliente
 */
static int inicializar_comunicacion(void) {
    /* Si ya está inicializado, retornar */
    if (cola_servidor != -1 && cola_cliente != -1) {
        return 0;
    }
    
    /* Abrir cola del servidor */
    if (cola_servidor == -1) {
        cola_servidor = mq_open(COLA_SERVIDOR, O_WRONLY);
        if (cola_servidor == (mqd_t)-1) {
            perror("[Proxy] Error al abrir cola del servidor");
            return -2;  /* Error de comunicación */
        }
    }
    
    /* Crear cola del cliente (única por proceso usando PID) */
    if (cola_cliente == -1) {
        struct mq_attr atributos;
        atributos.mq_flags = 0;
        atributos.mq_maxmsg = 10;
        atributos.mq_msgsize = sizeof(Respuesta);
        atributos.mq_curmsgs = 0;
        
        snprintf(nombre_cola_cliente, sizeof(nombre_cola_cliente), 
                 "%s%d", COLA_CLIENTE_PREFIX, getpid());
        
        /* Eliminar cola anterior si existe */
        mq_unlink(nombre_cola_cliente);
        
        cola_cliente = mq_open(nombre_cola_cliente, O_CREAT | O_RDONLY | O_EXCL, 
                               0666, &atributos);
        if (cola_cliente == (mqd_t)-1) {
            perror("[Proxy] Error al crear cola del cliente");
            if (cola_servidor != -1) {
                mq_close(cola_servidor);
                cola_servidor = -1;
            }
            return -2;  /* Error de comunicación */
        }
    }
    
    return 0;
}

/**
 * Función auxiliar: Envía una petición y espera la respuesta
 */
static int enviar_peticion_y_recibir_respuesta(Peticion* peticion, Respuesta* respuesta) {
    /* Inicializar comunicación si es necesario */
    if (inicializar_comunicacion() != 0) {
        return -2;
    }
    
    /* Configurar PID del cliente en la petición */
    peticion->tipo_mensaje = 1;
    peticion->pid_cliente = getpid();
    
    /* Enviar petición al servidor */
    if (mq_send(cola_servidor, (char*)peticion, sizeof(Peticion), 0) == -1) {
        perror("[Proxy] Error al enviar petición");
        return -2;  /* Error de comunicación */
    }
    
    /* Recibir respuesta del servidor (con timeout de 5 segundos) */
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5;
    
    ssize_t bytes = mq_timedreceive(cola_cliente, (char*)respuesta, 
                                     sizeof(Respuesta), NULL, &timeout);
    
    if (bytes == -1) {
        if (errno == ETIMEDOUT) {
            fprintf(stderr, "[Proxy] Timeout esperando respuesta del servidor\n");
        } else {
            perror("[Proxy] Error al recibir respuesta");
        }
        return -2;  /* Error de comunicación */
    }
    
    if (bytes != sizeof(Respuesta)) {
        fprintf(stderr, "[Proxy] Tamaño de respuesta incorrecto\n");
        return -2;  /* Error de comunicación */
    }
    
    return respuesta->resultado;
}

/**
 * Función auxiliar: Limpia las colas al terminar el proceso
 */
static void __attribute__((destructor)) limpiar_comunicacion(void) {
    if (cola_servidor != -1) {
        mq_close(cola_servidor);
        cola_servidor = -1;
    }
    
    if (cola_cliente != -1) {
        mq_close(cola_cliente);
        mq_unlink(nombre_cola_cliente);
        cola_cliente = -1;
    }
}

/**
 * destroy - Inicializa el servicio destruyendo todas las tuplas
 */
int destroy(void) {
    Peticion peticion;
    Respuesta respuesta;
    
    memset(&peticion, 0, sizeof(Peticion));
    memset(&respuesta, 0, sizeof(Respuesta));
    
    peticion.operacion = OP_DESTROY;
    
    return enviar_peticion_y_recibir_respuesta(&peticion, &respuesta);
}

/**
 * set_value - Inserta una nueva tupla
 */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validaciones locales (para evitar enviar peticiones inválidas) */
    if (key == NULL || strlen(key) > 255) {
        return -1;
    }
    if (value1 == NULL || strlen(value1) > 255) {
        return -1;
    }
    if (N_value2 < 1 || N_value2 > 32) {
        return -1;
    }
    if (V_value2 == NULL) {
        return -1;
    }
    
    Peticion peticion;
    Respuesta respuesta;
    
    memset(&peticion, 0, sizeof(Peticion));
    memset(&respuesta, 0, sizeof(Respuesta));
    
    peticion.operacion = OP_SET_VALUE;
    strncpy(peticion.key, key, MAX_KEY_LENGTH - 1);
    strncpy(peticion.value1, value1, MAX_VALUE1_LENGTH - 1);
    peticion.N_value2 = N_value2;
    memcpy(peticion.V_value2, V_value2, N_value2 * sizeof(float));
    peticion.value3 = value3;
    
    return enviar_peticion_y_recibir_respuesta(&peticion, &respuesta);
}

/**
 * get_value - Obtiene los valores asociados a una clave
 */
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    if (key == NULL || value1 == NULL || N_value2 == NULL || 
        V_value2 == NULL || value3 == NULL) {
        return -1;
    }
    
    Peticion peticion;
    Respuesta respuesta;
    
    memset(&peticion, 0, sizeof(Peticion));
    memset(&respuesta, 0, sizeof(Respuesta));
    
    peticion.operacion = OP_GET_VALUE;
    strncpy(peticion.key, key, MAX_KEY_LENGTH - 1);
    
    int resultado = enviar_peticion_y_recibir_respuesta(&peticion, &respuesta);
    
    /* Si la operación fue exitosa, copiar los datos recibidos */
    if (resultado == 0) {
        strncpy(value1, respuesta.value1, MAX_VALUE1_LENGTH - 1);
        value1[MAX_VALUE1_LENGTH - 1] = '\0';
        
        *N_value2 = respuesta.N_value2;
        memcpy(V_value2, respuesta.V_value2, respuesta.N_value2 * sizeof(float));
        *value3 = respuesta.value3;
    }
    
    return resultado;
}

/**
 * modify_value - Modifica los valores asociados a una clave existente
 */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validaciones locales */
    if (key == NULL || strlen(key) > 255) {
        return -1;
    }
    if (value1 == NULL || strlen(value1) > 255) {
        return -1;
    }
    if (N_value2 < 1 || N_value2 > 32) {
        return -1;
    }
    if (V_value2 == NULL) {
        return -1;
    }
    
    Peticion peticion;
    Respuesta respuesta;
    
    memset(&peticion, 0, sizeof(Peticion));
    memset(&respuesta, 0, sizeof(Respuesta));
    
    peticion.operacion = OP_MODIFY_VALUE;
    strncpy(peticion.key, key, MAX_KEY_LENGTH - 1);
    strncpy(peticion.value1, value1, MAX_VALUE1_LENGTH - 1);
    peticion.N_value2 = N_value2;
    memcpy(peticion.V_value2, V_value2, N_value2 * sizeof(float));
    peticion.value3 = value3;
    
    return enviar_peticion_y_recibir_respuesta(&peticion, &respuesta);
}

/**
 * delete_key - Elimina la tupla con la clave especificada
 */
int delete_key(char *key) {
    if (key == NULL) {
        return -1;
    }
    
    Peticion peticion;
    Respuesta respuesta;
    
    memset(&peticion, 0, sizeof(Peticion));
    memset(&respuesta, 0, sizeof(Respuesta));
    
    peticion.operacion = OP_DELETE_KEY;
    strncpy(peticion.key, key, MAX_KEY_LENGTH - 1);
    
    return enviar_peticion_y_recibir_respuesta(&peticion, &respuesta);
}

/**
 * exist - Verifica si existe una tupla con la clave especificada
 */
int exist(char *key) {
    if (key == NULL) {
        return -1;
    }
    
    Peticion peticion;
    Respuesta respuesta;
    
    memset(&peticion, 0, sizeof(Peticion));
    memset(&respuesta, 0, sizeof(Respuesta));
    
    peticion.operacion = OP_EXIST;
    strncpy(peticion.key, key, MAX_KEY_LENGTH - 1);
    
    return enviar_peticion_y_recibir_respuesta(&peticion, &respuesta);
}

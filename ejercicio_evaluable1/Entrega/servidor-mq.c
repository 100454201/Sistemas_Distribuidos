/*
 * servidor-mq.c - Servidor concurrente con colas de mensajes POSIX
 * Sistemas Distribuidos - Ejercicio Evaluable 1
 * 
 * Servidor que recibe peticiones por colas de mensajes,
 * las procesa invocando las funciones de libclaves.so,
 * y devuelve las respuestas.
 * 
 * Concurrencia: Hilos bajo demanda (un hilo por petición).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include "claves.h"
#include "protocolo.h"

/* Variables globales */
static mqd_t cola_servidor = -1;
static volatile int servidor_activo = 1;

/* Estructura para pasar datos al hilo */
typedef struct {
    Peticion peticion;
} DatosHilo;

/**
 * Manejador de señales para terminación limpia
 */
void manejador_señal(int sig) {
    (void)sig;
    printf("\n[Servidor] Recibida señal de terminación. Cerrando...\n");
    servidor_activo = 0;
}

/**
 * Función ejecutada por cada hilo: procesa una petición
 */
void* procesar_peticion(void* arg) {
    DatosHilo* datos = (DatosHilo*)arg;
    Peticion* pet = &datos->peticion;
    Respuesta resp;
    
    /* Inicializar respuesta */
    memset(&resp, 0, sizeof(Respuesta));
    resp.tipo_mensaje = 1;
    resp.resultado = -1;
    
    /* Procesar según la operación solicitada */
    switch (pet->operacion) {
        case OP_DESTROY:
            printf("[Servidor] Procesando DESTROY\n");
            resp.resultado = destroy();
            break;
            
        case OP_SET_VALUE:
            printf("[Servidor] Procesando SET_VALUE (key=%s)\n", pet->key);
            resp.resultado = set_value(pet->key, pet->value1, pet->N_value2, 
                                      pet->V_value2, pet->value3);
            break;
            
        case OP_GET_VALUE:
            printf("[Servidor] Procesando GET_VALUE (key=%s)\n", pet->key);
            resp.resultado = get_value(pet->key, resp.value1, &resp.N_value2, 
                                      resp.V_value2, &resp.value3);
            break;
            
        case OP_MODIFY_VALUE:
            printf("[Servidor] Procesando MODIFY_VALUE (key=%s)\n", pet->key);
            resp.resultado = modify_value(pet->key, pet->value1, pet->N_value2, 
                                         pet->V_value2, pet->value3);
            break;
            
        case OP_DELETE_KEY:
            printf("[Servidor] Procesando DELETE_KEY (key=%s)\n", pet->key);
            resp.resultado = delete_key(pet->key);
            break;
            
        case OP_EXIST:
            printf("[Servidor] Procesando EXIST (key=%s)\n", pet->key);
            resp.resultado = exist(pet->key);
            break;
            
        case OP_TERMINAR:
            printf("[Servidor] Petición de terminación recibida\n");
            servidor_activo = 0;
            free(datos);
            return NULL;
            
        default:
            printf("[Servidor] Operación desconocida: %d\n", pet->operacion);
            resp.resultado = -1;
            break;
    }
    
    /* Abrir cola del cliente para enviar respuesta */
    char nombre_cola_cliente[64];
    snprintf(nombre_cola_cliente, sizeof(nombre_cola_cliente), 
             "%s%d", COLA_CLIENTE_PREFIX, pet->pid_cliente);
    
    mqd_t cola_cliente = mq_open(nombre_cola_cliente, O_WRONLY);
    if (cola_cliente == (mqd_t)-1) {
        perror("[Servidor] Error al abrir cola del cliente");
        free(datos);
        return NULL;
    }
    
    /* Enviar respuesta */
    if (mq_send(cola_cliente, (char*)&resp, sizeof(Respuesta), 0) == -1) {
        perror("[Servidor] Error al enviar respuesta");
    } else {
        printf("[Servidor] Respuesta enviada (resultado=%d)\n", resp.resultado);
    }
    
    mq_close(cola_cliente);
    free(datos);
    return NULL;
}

/**
 * Función principal del servidor
 */
int main(void) {
    struct mq_attr atributos;
    Peticion peticion;
    
    printf("===========================================\n");
    printf("  SERVIDOR DE TUPLAS - Colas POSIX\n");
    printf("  Sistemas Distribuidos - Ejercicio 1\n");
    printf("===========================================\n\n");
    
    /* Configurar manejador de señales */
    signal(SIGINT, manejador_señal);
    signal(SIGTERM, manejador_señal);
    
    /* Inicializar el servicio de tuplas */
    printf("[Servidor] Inicializando servicio de tuplas...\n");
    if (destroy() != 0) {
        fprintf(stderr, "[Servidor] Error al inicializar el servicio\n");
    }
    
    /* Eliminar cola anterior si existe */
    mq_unlink(COLA_SERVIDOR);
    
    /* Configurar atributos de la cola */
    atributos.mq_flags = 0;
    atributos.mq_maxmsg = 10;
    atributos.mq_msgsize = sizeof(Peticion);
    atributos.mq_curmsgs = 0;
    
    /* Crear y abrir cola del servidor */
    printf("[Servidor] Creando cola: %s\n", COLA_SERVIDOR);
    cola_servidor = mq_open(COLA_SERVIDOR, O_CREAT | O_RDONLY, 0666, &atributos);
    if (cola_servidor == (mqd_t)-1) {
        perror("[Servidor] Error al crear/abrir cola del servidor");
        exit(EXIT_FAILURE);
    }
    
    printf("[Servidor] Servidor iniciado. Esperando peticiones...\n");
    printf("[Servidor] Presione Ctrl+C para terminar.\n\n");
    
    /* Bucle principal: esperar y procesar peticiones */
    while (servidor_activo) {
        /* Recibir petición (con timeout para permitir verificar servidor_activo) */
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;  /* Timeout de 1 segundo */
        
        ssize_t bytes_recibidos = mq_timedreceive(cola_servidor, (char*)&peticion, 
                                                   sizeof(Peticion), NULL, &timeout);
        
        if (bytes_recibidos == -1) {
            if (errno == ETIMEDOUT) {
                /* Timeout: continuar el bucle para verificar servidor_activo */
                continue;
            }
            if (errno == EINTR) {
                /* Interrupción por señal */
                continue;
            }
            perror("[Servidor] Error al recibir petición");
            continue;
        }
        
        if (bytes_recibidos != sizeof(Peticion)) {
            fprintf(stderr, "[Servidor] Tamaño de petición incorrecto\n");
            continue;
        }
        
        /* Crear estructura de datos para el hilo */
        DatosHilo* datos = (DatosHilo*)malloc(sizeof(DatosHilo));
        if (datos == NULL) {
            fprintf(stderr, "[Servidor] Error al reservar memoria para hilo\n");
            continue;
        }
        datos->peticion = peticion;
        
        /* Crear hilo para procesar la petición */
        pthread_t hilo;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if (pthread_create(&hilo, &attr, procesar_peticion, datos) != 0) {
            perror("[Servidor] Error al crear hilo");
            free(datos);
        }
        
        pthread_attr_destroy(&attr);
    }
    
    /* Limpieza y cierre */
    printf("\n[Servidor] Cerrando servidor...\n");
    
    if (cola_servidor != -1) {
        mq_close(cola_servidor);
        mq_unlink(COLA_SERVIDOR);
    }
    
    printf("[Servidor] Servidor terminado.\n");
    return 0;
}
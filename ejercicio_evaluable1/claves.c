/*
 * claves.c - Implementación del servicio de almacenamiento de tuplas
 * Sistemas Distribuidos - Ejercicio Evaluable 1
 * 
 * Implementación no distribuida usando lista enlazada dinámica.
 * Protección de concurrencia mediante mutex POSIX.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "claves.h"

#define MAX_KEY_LENGTH 256
#define MAX_VALUE1_LENGTH 256
#define MIN_N_VALUE2 1
#define MAX_N_VALUE2 32

/* Nodo de la lista enlazada que almacena una tupla */
typedef struct Nodo {
    char key[MAX_KEY_LENGTH];
    char value1[MAX_VALUE1_LENGTH];
    int N_value2;
    float V_value2[MAX_N_VALUE2];
    struct Paquete value3;
    struct Nodo *siguiente;
} Nodo;

/* Cabeza de la lista enlazada */
static Nodo *lista_tuplas = NULL;

/* Mutex para garantizar operaciones atómicas */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Función auxiliar: busca un nodo por clave
 * Precondición: el mutex debe estar bloqueado
 * Retorna el nodo si lo encuentra, NULL si no existe
 */
static Nodo* buscar_nodo(const char *key) {
    Nodo *actual = lista_tuplas;
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            return actual;
        }
        actual = actual->siguiente;
    }
    return NULL;
}

/*
 * Función auxiliar: valida los parámetros comunes
 * Retorna 0 si son válidos, -1 si no
 */
static int validar_parametros(const char *key, const char *value1, int N_value2) {
    if (key == NULL || strlen(key) > 255) {
        return -1;
    }
    if (value1 == NULL || strlen(value1) > 255) {
        return -1;
    }
    if (N_value2 < MIN_N_VALUE2 || N_value2 > MAX_N_VALUE2) {
        return -1;
    }
    return 0;
}

/**
 * destroy - Inicializa el servicio destruyendo todas las tuplas
 */
int destroy(void) {
    pthread_mutex_lock(&mutex);
    
    Nodo *actual = lista_tuplas;
    Nodo *siguiente;
    
    /* Liberar todos los nodos de la lista */
    while (actual != NULL) {
        siguiente = actual->siguiente;
        free(actual);
        actual = siguiente;
    }
    
    lista_tuplas = NULL;
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

/**
 * set_value - Inserta una nueva tupla
 */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validar parámetros antes de bloquear */
    if (validar_parametros(key, value1, N_value2) != 0) {
        return -1;
    }
    if (V_value2 == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex);
    
    /* Verificar que la clave no exista */
    if (buscar_nodo(key) != NULL) {
        pthread_mutex_unlock(&mutex);
        return -1; /* La clave ya existe */
    }
    
    /* Crear nuevo nodo */
    Nodo *nuevo = (Nodo *)malloc(sizeof(Nodo));
    if (nuevo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1; /* Error de memoria */
    }
    
    /* Copiar datos al nuevo nodo */
    strncpy(nuevo->key, key, MAX_KEY_LENGTH - 1);
    nuevo->key[MAX_KEY_LENGTH - 1] = '\0';
    
    strncpy(nuevo->value1, value1, MAX_VALUE1_LENGTH - 1);
    nuevo->value1[MAX_VALUE1_LENGTH - 1] = '\0';
    
    nuevo->N_value2 = N_value2;
    for (int i = 0; i < N_value2; i++) {
        nuevo->V_value2[i] = V_value2[i];
    }
    
    nuevo->value3 = value3;
    
    /* Insertar al principio de la lista */
    nuevo->siguiente = lista_tuplas;
    lista_tuplas = nuevo;
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

/**
 * get_value - Obtiene los valores asociados a una clave
 */
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex);
    
    Nodo *nodo = buscar_nodo(key);
    if (nodo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1; /* Clave no encontrada */
    }
    
    /* Copiar valores */
    strncpy(value1, nodo->value1, MAX_VALUE1_LENGTH - 1);
    value1[MAX_VALUE1_LENGTH - 1] = '\0';
    
    *N_value2 = nodo->N_value2;
    for (int i = 0; i < nodo->N_value2; i++) {
        V_value2[i] = nodo->V_value2[i];
    }
    
    *value3 = nodo->value3;
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

/**
 * modify_value - Modifica los valores asociados a una clave existente
 */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validar parámetros */
    if (validar_parametros(key, value1, N_value2) != 0) {
        return -1;
    }
    if (V_value2 == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex);
    
    Nodo *nodo = buscar_nodo(key);
    if (nodo == NULL) {
        pthread_mutex_unlock(&mutex);
        return -1; /* Clave no encontrada */
    }
    
    /* Modificar valores */
    strncpy(nodo->value1, value1, MAX_VALUE1_LENGTH - 1);
    nodo->value1[MAX_VALUE1_LENGTH - 1] = '\0';
    
    nodo->N_value2 = N_value2;
    for (int i = 0; i < N_value2; i++) {
        nodo->V_value2[i] = V_value2[i];
    }
    
    nodo->value3 = value3;
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

/**
 * delete_key - Elimina la tupla con la clave especificada
 */
int delete_key(char *key) {
    if (key == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex);
    
    Nodo *actual = lista_tuplas;
    Nodo *anterior = NULL;
    
    /* Buscar el nodo a eliminar */
    while (actual != NULL) {
        if (strcmp(actual->key, key) == 0) {
            /* Encontrado - eliminar */
            if (anterior == NULL) {
                /* Es el primer nodo */
                lista_tuplas = actual->siguiente;
            } else {
                /* No es el primer nodo */
                anterior->siguiente = actual->siguiente;
            }
            free(actual);
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
    
    /* No encontrado */
    pthread_mutex_unlock(&mutex);
    return -1;
}

/**
 * exist - Verifica si existe una tupla con la clave especificada
 */
int exist(char *key) {
    if (key == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&mutex);
    
    Nodo *nodo = buscar_nodo(key);
    int resultado = (nodo != NULL) ? 1 : 0;
    
    pthread_mutex_unlock(&mutex);
    return resultado;
}

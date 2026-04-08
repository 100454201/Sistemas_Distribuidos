/*
 * Servidor que acepta conexiones TCP, procesa peticiones
 * en texto plano invocando las funciones de libclaves.so,
 * y devuelve las respuestas al cliente.
 *
 * Concurrencia: Hilos bajo demanda (un hilo por conexion).
 * Protocolo: texto plano con campos separados por '\n'.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "claves.h"

#define MAX_LINE 512
#define BACKLOG  10

/* Socket del servidor (global para limpieza en senal) */
static int servidor_fd = -1;
static volatile int servidor_activo = 1;

/* ------------------------------------------------------------------ */
/* Utilidades de lectura/escritura sobre el socket                     */
/* ------------------------------------------------------------------ */

/*
 * leer_linea: lee caracteres del socket hasta encontrar '\n' o EOF.
 * Almacena la linea en buf (sin el '\n'), terminada en '\0'.
 * Devuelve el numero de caracteres leidos, 0 en EOF, -1 en error.
 */
static int leer_linea(int fd, char *buf, int maxlen) {
    int n = 0;
    char c;
    while (n < maxlen - 1) {
        int r = recv(fd, &c, 1, 0);
        if (r == 0) return 0;   /* EOF */
        if (r < 0) return -1;   /* error */
        if (c == '\n') break;
        buf[n++] = c;
    }
    buf[n] = '\0';
    return n;
}

/*
 * enviar_linea: envia una cadena seguida de '\n' por el socket.
 * Devuelve 0 en exito, -1 en error.
 */
static int enviar_linea(int fd, const char *msg) {
    char buf[MAX_LINE];
    int len = snprintf(buf, sizeof(buf), "%s\n", msg);
    if (send(fd, buf, len, 0) < 0) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Procesamiento de cada operacion                                     */
/* ------------------------------------------------------------------ */

static void manejar_destroy(int fd) {
    int res = destroy();
    char resp[16];
    snprintf(resp, sizeof(resp), "%d", res);
    enviar_linea(fd, resp);
}

static void manejar_set_value(int fd) {
    char key[256], value1[256], n_str[16], floats_str[MAX_LINE];
    char x_str[16], y_str[16], z_str[16];

    if (leer_linea(fd, key,        sizeof(key))        <= 0) return;
    if (leer_linea(fd, value1,     sizeof(value1))     <= 0) return;
    if (leer_linea(fd, n_str,      sizeof(n_str))      <= 0) return;
    if (leer_linea(fd, floats_str, sizeof(floats_str)) <= 0) return;
    if (leer_linea(fd, x_str,      sizeof(x_str))      <= 0) return;
    if (leer_linea(fd, y_str,      sizeof(y_str))      <= 0) return;
    if (leer_linea(fd, z_str,      sizeof(z_str))      <= 0) return;

    int N = atoi(n_str);
    float V[32];
    char *token = strtok(floats_str, " ");
    for (int i = 0; i < N && token != NULL; i++) {
        V[i] = atof(token);
        token = strtok(NULL, " ");
    }

    struct Paquete p;
    p.x = atoi(x_str);
    p.y = atoi(y_str);
    p.z = atoi(z_str);

    int res = set_value(key, value1, N, V, p);
    char resp[16];
    snprintf(resp, sizeof(resp), "%d", res);
    enviar_linea(fd, resp);
}

static void manejar_get_value(int fd) {
    char key[256];
    if (leer_linea(fd, key, sizeof(key)) <= 0) return;

    char value1[256];
    int N;
    float V[32];
    struct Paquete p;

    int res = get_value(key, value1, &N, V, &p);

    char resp[16];
    snprintf(resp, sizeof(resp), "%d", res);
    enviar_linea(fd, resp);

    if (res == 0) {
        /* Enviar value1 */
        enviar_linea(fd, value1);

        /* Enviar N */
        char n_str[16];
        snprintf(n_str, sizeof(n_str), "%d", N);
        enviar_linea(fd, n_str);

        /* Enviar vector de floats separados por espacios */
        char floats_str[MAX_LINE];
        int offset = 0;
        for (int i = 0; i < N; i++) {
            offset += snprintf(floats_str + offset,
                               sizeof(floats_str) - offset,
                               i < N - 1 ? "%f " : "%f", V[i]);
        }
        enviar_linea(fd, floats_str);

        /* Enviar paquete x, y, z en lineas separadas */
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%d", p.x); enviar_linea(fd, tmp);
        snprintf(tmp, sizeof(tmp), "%d", p.y); enviar_linea(fd, tmp);
        snprintf(tmp, sizeof(tmp), "%d", p.z); enviar_linea(fd, tmp);
    }
}

static void manejar_modify_value(int fd) {
    char key[256], value1[256], n_str[16], floats_str[MAX_LINE];
    char x_str[16], y_str[16], z_str[16];

    if (leer_linea(fd, key,        sizeof(key))        <= 0) return;
    if (leer_linea(fd, value1,     sizeof(value1))     <= 0) return;
    if (leer_linea(fd, n_str,      sizeof(n_str))      <= 0) return;
    if (leer_linea(fd, floats_str, sizeof(floats_str)) <= 0) return;
    if (leer_linea(fd, x_str,      sizeof(x_str))      <= 0) return;
    if (leer_linea(fd, y_str,      sizeof(y_str))      <= 0) return;
    if (leer_linea(fd, z_str,      sizeof(z_str))      <= 0) return;

    int N = atoi(n_str);
    float V[32];
    char *token = strtok(floats_str, " ");
    for (int i = 0; i < N && token != NULL; i++) {
        V[i] = atof(token);
        token = strtok(NULL, " ");
    }

    struct Paquete p;
    p.x = atoi(x_str);
    p.y = atoi(y_str);
    p.z = atoi(z_str);

    int res = modify_value(key, value1, N, V, p);
    char resp[16];
    snprintf(resp, sizeof(resp), "%d", res);
    enviar_linea(fd, resp);
}

static void manejar_delete_key(int fd) {
    char key[256];
    if (leer_linea(fd, key, sizeof(key)) <= 0) return;

    int res = delete_key(key);
    char resp[16];
    snprintf(resp, sizeof(resp), "%d", res);
    enviar_linea(fd, resp);
}

static void manejar_exist(int fd) {
    char key[256];
    if (leer_linea(fd, key, sizeof(key)) <= 0) return;

    int res = exist(key);
    char resp[16];
    snprintf(resp, sizeof(resp), "%d", res);
    enviar_linea(fd, resp);
}

/* ------------------------------------------------------------------ */
/* Funcion del hilo: atiende una conexion completa                     */
/* ------------------------------------------------------------------ */

void *atender_cliente(void *arg) {
    int cliente_fd = *(int *)arg;
    free(arg);

    char operacion[MAX_LINE];

    if (leer_linea(cliente_fd, operacion, sizeof(operacion)) <= 0) {
        close(cliente_fd);
        return NULL;
    }

    printf("[Servidor] Conexion recibida. Operacion: %s\n", operacion);

    if      (strcmp(operacion, "DESTROY")      == 0) manejar_destroy(cliente_fd);
    else if (strcmp(operacion, "SET_VALUE")    == 0) manejar_set_value(cliente_fd);
    else if (strcmp(operacion, "GET_VALUE")    == 0) manejar_get_value(cliente_fd);
    else if (strcmp(operacion, "MODIFY_VALUE") == 0) manejar_modify_value(cliente_fd);
    else if (strcmp(operacion, "DELETE_KEY")   == 0) manejar_delete_key(cliente_fd);
    else if (strcmp(operacion, "EXIST")        == 0) manejar_exist(cliente_fd);
    else {
        printf("[Servidor] Operacion desconocida: %s\n", operacion);
        enviar_linea(cliente_fd, "-1");
    }

    close(cliente_fd);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Manejador de senales                                                */
/* ------------------------------------------------------------------ */

void manejador_senal(int sig) {
    (void)sig;
    printf("\n[Servidor] Senal recibida. Cerrando...\n");
    servidor_activo = 0;
    if (servidor_fd != -1) close(servidor_fd);
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <PUERTO>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int puerto = atoi(argv[1]);
    if (puerto <= 0 || puerto > 65535) {
        fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* Configurar senales */
    signal(SIGINT,  manejador_senal);
    signal(SIGTERM, manejador_senal);

    /* Inicializar servicio de tuplas */
    printf("===========================================\n");
    printf("  SERVIDOR DE TUPLAS - Sockets TCP\n");
    printf("  Sistemas Distribuidos - Ejercicio 2\n");
    printf("===========================================\n\n");

    printf("[Servidor] Inicializando servicio de tuplas...\n");
    destroy();

    /* Crear socket del servidor */
    servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_fd < 0) {
        perror("[Servidor] Error al crear socket");
        exit(EXIT_FAILURE);
    }

    /* Permitir reutilizar el puerto inmediatamente tras reinicio */
    int opt = 1;
    setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Asociar socket al puerto */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(puerto);

    if (bind(servidor_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[Servidor] Error en bind");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(servidor_fd, BACKLOG) < 0) {
        perror("[Servidor] Error en listen");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    printf("[Servidor] Escuchando en puerto %d...\n", puerto);
    printf("[Servidor] Presione Ctrl+C para terminar.\n\n");

    /* Bucle principal */
    while (servidor_activo) {
        struct sockaddr_in cliente_addr;
        socklen_t cliente_len = sizeof(cliente_addr);

        int *cliente_fd = malloc(sizeof(int));
        if (cliente_fd == NULL) {
            fprintf(stderr, "[Servidor] Error al reservar memoria\n");
            continue;
        }

        *cliente_fd = accept(servidor_fd,
                             (struct sockaddr *)&cliente_addr,
                             &cliente_len);

        if (*cliente_fd < 0) {
            free(cliente_fd);
            if (servidor_activo) perror("[Servidor] Error en accept");
            continue;
        }

        printf("[Servidor] Nueva conexion desde %s:%d\n",
               inet_ntoa(cliente_addr.sin_addr),
               ntohs(cliente_addr.sin_port));

        /* Crear hilo para atender al cliente */
        pthread_t hilo;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&hilo, &attr, atender_cliente, cliente_fd) != 0) {
            perror("[Servidor] Error al crear hilo");
            close(*cliente_fd);
            free(cliente_fd);
        }

        pthread_attr_destroy(&attr);
    }

    printf("[Servidor] Servidor terminado.\n");
    return 0;
}
/*
 * proxy-sock.c - Proxy del lado del cliente (implementacion de la API)
 * Sistemas Distribuidos - Ejercicio Evaluable 2
 *
 * Implementa las funciones de la API del servicio de tuplas
 * comunicandose con el servidor mediante sockets TCP.
 *
 * La direccion IP y el puerto del servidor se leen de las
 * variables de entorno IP_TUPLAS y PORT_TUPLAS.
 *
 * Protocolo: texto plano con campos separados por '\n'.
 * Este codigo se compila en libproxyclaves.so.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "claves.h"

#define MAX_LINE 512

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
        if (r == 0) return 0;
        if (r < 0) return -1;
        if (c == '\n') break;
        buf[n++] = c;
    }
    buf[n] = '\0';
    return n;
}

    /*
     * enviar_todo: garantiza el envio completo de len bytes.
     * Devuelve 0 en exito, -1 en error.
     */
    static int enviar_todo(int fd, const char *data, size_t len) {
        size_t enviados = 0;
        while (enviados < len) {
            ssize_t r = send(fd, data + enviados, len - enviados, 0);
            if (r < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return -1;
            }
            if (r == 0) {
                return -1;
            }
            enviados += (size_t)r;
        }
        return 0;
    }

/*
 * enviar_linea: envia una cadena seguida de '\n' por el socket.
 * Devuelve 0 en exito, -1 en error.
 */
static int enviar_linea(int fd, const char *msg) {
    char buf[MAX_LINE];
    int len = snprintf(buf, sizeof(buf), "%s\n", msg);
        if (len < 0 || len >= (int)sizeof(buf)) return -1;
        if (enviar_todo(fd, buf, (size_t)len) < 0) return -1;
    return 0;
}

/*
 * construir_lista_floats: serializa N floats en una linea separada por espacios.
 * Devuelve 0 en exito, -1 si el buffer no es suficiente.
 */
static int construir_lista_floats(char *dest, size_t dest_size, const float *values, int N) {
    size_t usado = 0;

    if (dest == NULL || values == NULL || dest_size == 0) {
        return -1;
    }

    dest[0] = '\0';

    for (int i = 0; i < N; i++) {
        int escritos = snprintf(dest + usado, dest_size - usado,
                                (i < N - 1) ? "%f " : "%f", values[i]);
        if (escritos < 0) {
            return -1;
        }
        if ((size_t)escritos >= dest_size - usado) {
            return -1;
        }
        usado += (size_t)escritos;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Funcion auxiliar: abre conexion TCP con el servidor                 */
/* ------------------------------------------------------------------ */

/*
 * conectar_servidor: lee IP_TUPLAS y PORT_TUPLAS del entorno,
 * crea un socket TCP y se conecta al servidor.
 * Devuelve el fd del socket en exito, -2 en error de comunicacion.
 */
static int conectar_servidor(void) {
    char *ip   = getenv("IP_TUPLAS");
    char *port = getenv("PORT_TUPLAS");

    if (ip == NULL || port == NULL) {
        fprintf(stderr, "[Proxy] Variables IP_TUPLAS o PORT_TUPLAS no definidas\n");
        return -2;
    }

    int puerto = atoi(port);
    if (puerto <= 0 || puerto > 65535) {
        fprintf(stderr, "[Proxy] Puerto invalido: %s\n", port);
        return -2;
    }

    /* Resolver nombre o direccion IP */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, &res) != 0) {
        fprintf(stderr, "[Proxy] No se pudo resolver la direccion: %s\n", ip);
        return -2;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        perror("[Proxy] Error al crear socket");
        freeaddrinfo(res);
        return -2;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("[Proxy] Error al conectar con el servidor");
        close(fd);
        freeaddrinfo(res);
        return -2;
    }

    freeaddrinfo(res);
    return fd;
}

/* ------------------------------------------------------------------ */
/* Implementacion de la API                                            */
/* ------------------------------------------------------------------ */

int destroy(void) {
    int fd = conectar_servidor();
    if (fd < 0) return -2;

    if (enviar_linea(fd, "DESTROY") < 0) { close(fd); return -2; }

    char resp[16];
    if (leer_linea(fd, resp, sizeof(resp)) <= 0) { close(fd); return -2; }

    close(fd);
    return atoi(resp);
}

int set_value(char *key, char *value1, int N_value2,
              float *V_value2, struct Paquete value3) {
    /* Validaciones locales */
    if (key    == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;
    if (value1 == NULL || strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32)          return -1;
    if (V_value2 == NULL)                        return -1;

    int fd = conectar_servidor();
    if (fd < 0) return -2;

    /* Enviar operacion y parametros */
    if (enviar_linea(fd, "SET_VALUE") < 0) { close(fd); return -2; }
    if (enviar_linea(fd, key)         < 0) { close(fd); return -2; }
    if (enviar_linea(fd, value1)      < 0) { close(fd); return -2; }

    /* N */
    char n_str[16];
    snprintf(n_str, sizeof(n_str), "%d", N_value2);
    if (enviar_linea(fd, n_str) < 0) { close(fd); return -2; }

    /* Vector de floats en una sola linea separados por espacios */
    char floats_str[MAX_LINE];
    if (construir_lista_floats(floats_str, sizeof(floats_str), V_value2, N_value2) < 0) {
        close(fd);
        return -2;
    }
    if (enviar_linea(fd, floats_str) < 0) { close(fd); return -2; }

    /* Paquete: x, y, z en lineas separadas */
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%d", value3.x);
    if (enviar_linea(fd, tmp) < 0) { close(fd); return -2; }
    snprintf(tmp, sizeof(tmp), "%d", value3.y);
    if (enviar_linea(fd, tmp) < 0) { close(fd); return -2; }
    snprintf(tmp, sizeof(tmp), "%d", value3.z);
    if (enviar_linea(fd, tmp) < 0) { close(fd); return -2; }

    /* Leer respuesta */
    char resp[16];
    if (leer_linea(fd, resp, sizeof(resp)) <= 0) { close(fd); return -2; }

    close(fd);
    return atoi(resp);
}

int get_value(char *key, char *value1, int *N_value2,
              float *V_value2, struct Paquete *value3) {
    if (key    == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;
    if (value1 == NULL || N_value2 == NULL)  return -1;
    if (V_value2 == NULL || value3 == NULL)  return -1;

    int fd = conectar_servidor();
    if (fd < 0) return -2;

    if (enviar_linea(fd, "GET_VALUE") < 0) { close(fd); return -2; }
    if (enviar_linea(fd, key)         < 0) { close(fd); return -2; }

    /* Leer codigo de resultado */
    char resp[16];
    if (leer_linea(fd, resp, sizeof(resp)) <= 0) { close(fd); return -2; }
    int resultado = atoi(resp);

    if (resultado == 0) {
        /* Leer value1 */
        if (leer_linea(fd, value1, 256) < 0) { close(fd); return -2; }

        /* Leer N */
        char n_str[16];
        if (leer_linea(fd, n_str, sizeof(n_str)) <= 0) { close(fd); return -2; }
        *N_value2 = atoi(n_str);
        if (*N_value2 < 1 || *N_value2 > 32) { close(fd); return -2; }

        /* Leer vector de floats */
        char floats_str[MAX_LINE];
        if (leer_linea(fd, floats_str, sizeof(floats_str)) <= 0) { close(fd); return -2; }
        char *token = strtok(floats_str, " ");
        int parseados = 0;
        while (parseados < *N_value2 && token != NULL) {
            V_value2[parseados] = atof(token);
            parseados++;
            token = strtok(NULL, " ");
        }
        if (parseados != *N_value2) { close(fd); return -2; }

        /* Leer paquete x, y, z */
        char x_str[16], y_str[16], z_str[16];
        if (leer_linea(fd, x_str, sizeof(x_str)) <= 0) { close(fd); return -2; }
        if (leer_linea(fd, y_str, sizeof(y_str)) <= 0) { close(fd); return -2; }
        if (leer_linea(fd, z_str, sizeof(z_str)) <= 0) { close(fd); return -2; }
        value3->x = atoi(x_str);
        value3->y = atoi(y_str);
        value3->z = atoi(z_str);
    }

    close(fd);
    return resultado;
}

int modify_value(char *key, char *value1, int N_value2,
                 float *V_value2, struct Paquete value3) {
    /* Validaciones locales */
    if (key    == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;
    if (value1 == NULL || strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32)          return -1;
    if (V_value2 == NULL)                        return -1;

    int fd = conectar_servidor();
    if (fd < 0) return -2;

    if (enviar_linea(fd, "MODIFY_VALUE") < 0) { close(fd); return -2; }
    if (enviar_linea(fd, key)            < 0) { close(fd); return -2; }
    if (enviar_linea(fd, value1)         < 0) { close(fd); return -2; }

    char n_str[16];
    snprintf(n_str, sizeof(n_str), "%d", N_value2);
    if (enviar_linea(fd, n_str) < 0) { close(fd); return -2; }

    char floats_str[MAX_LINE];
    if (construir_lista_floats(floats_str, sizeof(floats_str), V_value2, N_value2) < 0) {
        close(fd);
        return -2;
    }
    if (enviar_linea(fd, floats_str) < 0) { close(fd); return -2; }

    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%d", value3.x);
    if (enviar_linea(fd, tmp) < 0) { close(fd); return -2; }
    snprintf(tmp, sizeof(tmp), "%d", value3.y);
    if (enviar_linea(fd, tmp) < 0) { close(fd); return -2; }
    snprintf(tmp, sizeof(tmp), "%d", value3.z);
    if (enviar_linea(fd, tmp) < 0) { close(fd); return -2; }

    char resp[16];
    if (leer_linea(fd, resp, sizeof(resp)) <= 0) { close(fd); return -2; }

    close(fd);
    return atoi(resp);
}

int delete_key(char *key) {
    if (key == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;

    int fd = conectar_servidor();
    if (fd < 0) return -2;

    if (enviar_linea(fd, "DELETE_KEY") < 0) { close(fd); return -2; }
    if (enviar_linea(fd, key)          < 0) { close(fd); return -2; }

    char resp[16];
    if (leer_linea(fd, resp, sizeof(resp)) <= 0) { close(fd); return -2; }

    close(fd);
    return atoi(resp);
}

int exist(char *key) {
    if (key == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;

    int fd = conectar_servidor();
    if (fd < 0) return -2;

    if (enviar_linea(fd, "EXIST") < 0) { close(fd); return -2; }
    if (enviar_linea(fd, key)     < 0) { close(fd); return -2; }

    char resp[16];
    if (leer_linea(fd, resp, sizeof(resp)) <= 0) { close(fd); return -2; }

    close(fd);
    return atoi(resp);
}
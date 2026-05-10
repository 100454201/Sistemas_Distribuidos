/*
 * Servidor concurrente multihilo que gestiona el registro,
 * conexion y envio de mensajes entre usuarios.
 * Uso: ./server -p <puerto>
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
#include <netdb.h>
#include "log_rpc.h"

#define MAX_NAME  256
#define MAX_MSG   256
#define MAX_FILE  256
#define MAX_PORT  6
#define MAX_IP    16
#define BACKLOG   10

/* Mensaje pendiente de entrega */
typedef struct Mensaje {
    unsigned int   id;
    char           remitente[MAX_NAME];
    char           texto[MAX_MSG];
    char           fichero[MAX_FILE]; /* "" si no tiene fichero adjunto */
    struct Mensaje *siguiente;
} Mensaje;

/* Usuario registrado en el sistema */
typedef struct Usuario {
    char            nombre[MAX_NAME];
    int             conectado;       /* 0=desconectado, 1=conectado */
    char            ip[MAX_IP];
    char            puerto[MAX_PORT];
    unsigned int    ultimo_id;       /* ultimo id de mensaje asignado */
    Mensaje        *mensajes;        /* lista de mensajes pendientes */
    struct Usuario *siguiente;
} Usuario;

/* Variables globales */
static Usuario         *lista_usuarios = NULL;
static pthread_mutex_t  mutex          = PTHREAD_MUTEX_INITIALIZER;
static int              servidor_fd    = -1;

/* Cliente RPC para el servicio de log (NULL si no esta configurado) */
static CLIENT          *rpc_cliente    = NULL;
static pthread_mutex_t  rpc_mutex      = PTHREAD_MUTEX_INITIALIZER;

/* ------------------------------------------------------------------ */
/* Cliente RPC: log de operaciones                                    */
/* ------------------------------------------------------------------ */

/*
 * rpc_log: envia al servidor RPC el usuario, la operacion y el fichero.
 * Si rpc_cliente es NULL (LOG_RPC_IP no definida) no hace nada.
 * El campo fichero puede ser NULL o "" para operaciones sin adjunto.
 * Usa rpc_mutex para garantizar acceso exclusivo al handle RPC.
 */
static void rpc_log(const char *usuario, const char *operacion,
                    const char *fichero) {
    if (rpc_cliente == NULL) return;

    LogPeticion peticion;
    peticion.usuario   = (char *)usuario;
    peticion.operacion = (char *)operacion;
    peticion.fichero   = (fichero != NULL && fichero[0] != '\0')
                         ? (char *)fichero : "";

    pthread_mutex_lock(&rpc_mutex);
    int *resultado = log_operacion_1(&peticion, rpc_cliente);
    pthread_mutex_unlock(&rpc_mutex);

    if (resultado == NULL) {
        clnt_perror(rpc_cliente, "s> RPC log error");
    }
}

/* ------------------------------------------------------------------ */
/* Funciones auxiliares lista de usuarios                             */
/* ------------------------------------------------------------------ */

/*
 * buscar_usuario: busca un usuario por nombre.
 * Precondicion: mutex debe estar bloqueado.
 * Devuelve puntero al usuario o NULL si no existe.
 */
static Usuario *buscar_usuario(const char *nombre) {
    Usuario *u = lista_usuarios;
    while (u != NULL) {
        if (strcmp(u->nombre, nombre) == 0) return u;
        u = u->siguiente;
    }
    return NULL;
}

/*
 * agregar_usuario: crea un nuevo usuario y lo inserta en la lista.
 * Precondicion: mutex debe estar bloqueado.
 * Devuelve 0 en exito, -1 en error de memoria.
 */
static int agregar_usuario(const char *nombre) {
    Usuario *nuevo = (Usuario *)malloc(sizeof(Usuario));
    if (nuevo == NULL) return -1;
    strncpy(nuevo->nombre, nombre, MAX_NAME - 1);
    nuevo->nombre[MAX_NAME - 1] = '\0';
    nuevo->conectado  = 0;
    nuevo->ip[0]      = '\0';
    nuevo->puerto[0]  = '\0';
    nuevo->ultimo_id  = 0;
    nuevo->mensajes   = NULL;
    nuevo->siguiente  = lista_usuarios;
    lista_usuarios    = nuevo;
    return 0;
}

/*
 * eliminar_mensajes: libera todos los mensajes pendientes de un usuario.
 * Precondicion: mutex debe estar bloqueado.
 */
static void eliminar_mensajes(Usuario *u) {
    Mensaje *m = u->mensajes;
    while (m != NULL) {
        Mensaje *sig = m->siguiente;
        free(m);
        m = sig;
    }
    u->mensajes = NULL;
}

/*
 * eliminar_usuario: borra un usuario de la lista y libera su memoria.
 * Precondicion: mutex debe estar bloqueado.
 * Devuelve 0 en exito, -1 si no existe.
 */
static int eliminar_usuario(const char *nombre) {
    Usuario *actual = lista_usuarios, *anterior = NULL;
    while (actual != NULL) {
        if (strcmp(actual->nombre, nombre) == 0) {
            if (anterior == NULL) lista_usuarios      = actual->siguiente;
            else                  anterior->siguiente = actual->siguiente;
            eliminar_mensajes(actual);
            free(actual);
            return 0;
        }
        anterior = actual;
        actual   = actual->siguiente;
    }
    return -1;
}

/*
 * agregar_mensaje: crea un mensaje pendiente para un usuario destino.
 * Si fich no es NULL ni vacio, almacena el nombre de fichero adjunto.
 * Precondicion: mutex debe estar bloqueado.
 * Devuelve el id asignado, 0 en error de memoria.
 */
static unsigned int agregar_mensaje(Usuario *dest, const char *rem,
                                     const char *txt, const char *fich) {
    Mensaje *nuevo = (Mensaje *)malloc(sizeof(Mensaje));
    if (nuevo == NULL) return 0;

    dest->ultimo_id++;
    if (dest->ultimo_id == 0) dest->ultimo_id = 1; /* evitar 0 */

    nuevo->id = dest->ultimo_id;
    strncpy(nuevo->remitente, rem, MAX_NAME - 1);
    nuevo->remitente[MAX_NAME - 1] = '\0';
    strncpy(nuevo->texto, txt, MAX_MSG - 1);
    nuevo->texto[MAX_MSG - 1] = '\0';

    /* Almacenar nombre de fichero adjunto si se proporciona */
    if (fich != NULL && fich[0] != '\0') {
        strncpy(nuevo->fichero, fich, MAX_FILE - 1);
        nuevo->fichero[MAX_FILE - 1] = '\0';
    } else {
        nuevo->fichero[0] = '\0';
    }

    nuevo->siguiente = NULL;

    /* Insertar al final */
    if (dest->mensajes == NULL) {
        dest->mensajes = nuevo;
    } else {
        Mensaje *m = dest->mensajes;
        while (m->siguiente != NULL) m = m->siguiente;
        m->siguiente = nuevo;
    }
    return nuevo->id;
}

/* ------------------------------------------------------------------ */
/* Utilidades de comunicacion por socket                              */
/* ------------------------------------------------------------------ */

/*
 * enviar_cadena: envia una cadena terminada en '\0' por el socket.
 * Devuelve 0 en exito, -1 en error.
 */
static int enviar_cadena(int fd, const char *cad) {
    int len = strlen(cad) + 1; /* incluye '\0' */
    if (send(fd, cad, len, 0) < 0) return -1;
    return 0;
}

/*
 * recibir_cadena: recibe una cadena terminada en '\0' del socket.
 * Lee byte a byte hasta encontrar '\0'.
 * Devuelve 0 en exito, -1 en error.
 */
static int recibir_cadena(int fd, char *buf, int maxlen) {
    int  n = 0;
    char c;
    while (n < maxlen - 1) {
        int r = recv(fd, &c, 1, 0);
        if (r <= 0) return -1;
        buf[n++] = c;
        if (c == '\0') break;
    }
    buf[n] = '\0';
    return 0;
}

/*
 * enviar_byte: envia un byte de resultado por el socket.
 * Devuelve 0 en exito, -1 en error.
 */
static int enviar_byte(int fd, char val) {
    if (send(fd, &val, 1, 0) < 0) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Entrega de mensajes servidor -> cliente                            */
/* ------------------------------------------------------------------ */

/*
 * conectar_a_cliente: abre una conexion TCP a la IP y puerto del cliente.
 * Devuelve el fd del socket en exito, -1 en error.
 */
static int conectar_a_cliente(const char *ip, const char *puerto) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(ip, puerto, &hints, &res) != 0) return -1;
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return -1; }
    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd); freeaddrinfo(res); return -1;
    }
    freeaddrinfo(res);
    return fd;
}

/*
 * entregar_mensaje: conecta al hilo de escucha del cliente destino y entrega
 * el mensaje. Si fichero no es vacio usa SEND_MESSAGE_ATTACH (seccion 2.3),
 * si no usa SEND_MESSAGE (seccion 8.6 parte 1).
 * Devuelve 0 en exito, -1 en error.
 */
static int entregar_mensaje(const char *ip, const char *puerto,
                             const char *remitente, unsigned int id,
                             const char *texto, const char *fichero) {
    int fd = conectar_a_cliente(ip, puerto);
    if (fd < 0) return -1;

    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%u", id);

    if (fichero != NULL && fichero[0] != '\0') {
        /* Mensaje con fichero adjunto */
        if (enviar_cadena(fd, "SEND_MESSAGE_ATTACH") < 0 ||
            enviar_cadena(fd, remitente)             < 0 ||
            enviar_cadena(fd, id_str)                < 0 ||
            enviar_cadena(fd, texto)                 < 0 ||
            enviar_cadena(fd, fichero)               < 0) {
            close(fd);
            return -1;
        }
    } else {
        /* Mensaje sin adjunto */
        if (enviar_cadena(fd, "SEND_MESSAGE") < 0 ||
            enviar_cadena(fd, remitente)      < 0 ||
            enviar_cadena(fd, id_str)         < 0 ||
            enviar_cadena(fd, texto)          < 0) {
            close(fd);
            return -1;
        }
    }
    close(fd);
    return 0;
}

/*
 * notificar_remitente: notifica al remitente que su mensaje fue entregado.
 * Si fichero no es vacio usa SEND_MESS_ATTACH_ACK, si no usa SEND_MESS_ACK.
 */
static void notificar_remitente(const char *ip, const char *puerto,
                                 unsigned int id, const char *fichero) {
    int fd = conectar_a_cliente(ip, puerto);
    if (fd < 0) return;

    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%u", id);

    if (fichero != NULL && fichero[0] != '\0') {
        /* ACK con nombre de fichero adjunto */
        enviar_cadena(fd, "SEND_MESS_ATTACH_ACK");
        enviar_cadena(fd, id_str);
        enviar_cadena(fd, fichero);
    } else {
        enviar_cadena(fd, "SEND_MESS_ACK");
        enviar_cadena(fd, id_str);
    }
    close(fd);
}

/* ------------------------------------------------------------------ */
/* Operaciones                                                         */
/* ------------------------------------------------------------------ */

/*
 * op_register: registra un nuevo usuario en el sistema.
 * Protocolo seccion 8.1.
 * Respuesta: 0 exito, 1 ya existe, 2 error.
 */
static void op_register(int fd, const char *ip) {
    (void)ip; /* no se usa en register pero se pasa por consistencia */
    char nombre[MAX_NAME];
    if (recibir_cadena(fd, nombre, sizeof(nombre)) < 0) {
        enviar_byte(fd, 2);
        return;
    }

    rpc_log(nombre, "REGISTER", NULL);

    pthread_mutex_lock(&mutex);

    if (buscar_usuario(nombre) != NULL) {
        pthread_mutex_unlock(&mutex);
        printf("s> REGISTER %s FAIL\n", nombre);
        enviar_byte(fd, 1); /* ya existe */
        return;
    }

    if (agregar_usuario(nombre) < 0) {
        pthread_mutex_unlock(&mutex);
        printf("s> REGISTER %s FAIL\n", nombre);
        enviar_byte(fd, 2); /* error de memoria */
        return;
    }

    pthread_mutex_unlock(&mutex);
    printf("s> REGISTER %s OK\n", nombre);
    enviar_byte(fd, 0);
}

/*
 * op_unregister: da de baja a un usuario del sistema.
 * Protocolo seccion 8.2.
 * Respuesta: 0 exito, 1 no existe, 2 error.
 */
static void op_unregister(int fd) {
    char nombre[MAX_NAME];
    if (recibir_cadena(fd, nombre, sizeof(nombre)) < 0) {
        enviar_byte(fd, 2);
        return;
    }

    rpc_log(nombre, "UNREGISTER", NULL);

    pthread_mutex_lock(&mutex);

    if (buscar_usuario(nombre) == NULL) {
        pthread_mutex_unlock(&mutex);
        printf("s> UNREGISTER %s FAIL\n", nombre);
        enviar_byte(fd, 1); /* no existe */
        return;
    }

    eliminar_usuario(nombre);
    pthread_mutex_unlock(&mutex);
    printf("s> UNREGISTER %s OK\n", nombre);
    enviar_byte(fd, 0);
}

/*
 * op_connect: conecta un usuario al sistema y entrega sus mensajes pendientes.
 * Recibe nombre y puerto de escucha del cliente; la IP se obtiene del accept.
 * Protocolo seccion 8.3.
 * Respuesta: 0 exito, 1 no existe, 2 ya conectado, 3 error.
 */
static void op_connect(int fd, const char *ip_cliente) {
    char nombre[MAX_NAME];
    char puerto[MAX_PORT];

    if (recibir_cadena(fd, nombre, sizeof(nombre)) < 0 ||
        recibir_cadena(fd, puerto, sizeof(puerto)) < 0) {
        enviar_byte(fd, 3);
        return;
    }

    rpc_log(nombre, "CONNECT", NULL);

    pthread_mutex_lock(&mutex);

    Usuario *u = buscar_usuario(nombre);
    if (u == NULL) {
        pthread_mutex_unlock(&mutex);
        printf("s> CONNECT %s FAIL\n", nombre);
        enviar_byte(fd, 1); /* no existe */
        return;
    }
    if (u->conectado) {
        pthread_mutex_unlock(&mutex);
        printf("s> CONNECT %s FAIL\n", nombre);
        enviar_byte(fd, 2); /* ya conectado */
        return;
    }

    /* Actualizar estado del usuario */
    u->conectado = 1;
    strncpy(u->ip,     ip_cliente, MAX_IP   - 1);
    strncpy(u->puerto, puerto,     MAX_PORT - 1);
    u->ip[MAX_IP - 1]       = '\0';
    u->puerto[MAX_PORT - 1] = '\0';

    /* Copiar mensajes pendientes para entregarlos fuera del mutex */
    Mensaje *pendientes = u->mensajes;
    u->mensajes = NULL;
    char ip_copia[MAX_IP], puerto_copia[MAX_PORT];
    strncpy(ip_copia,     u->ip,     MAX_IP   - 1);
    strncpy(puerto_copia, u->puerto, MAX_PORT - 1);
    ip_copia[MAX_IP - 1]       = '\0';
    puerto_copia[MAX_PORT - 1] = '\0';

    pthread_mutex_unlock(&mutex);

    printf("s> CONNECT %s OK\n", nombre);
    enviar_byte(fd, 0);

    /* Entregar mensajes pendientes uno a uno usando el protocolo adecuado */
    Mensaje *m = pendientes;
    while (m != NULL) {
        Mensaje *sig = m->siguiente;
        printf("s> SEND MESSAGE %u FROM %s TO %s\n",
               m->id, m->remitente, nombre);

        if (entregar_mensaje(ip_copia, puerto_copia,
                             m->remitente, m->id, m->texto, m->fichero) < 0) {
            /* Fallo: marcar usuario como desconectado y reponer mensajes */
            pthread_mutex_lock(&mutex);
            Usuario *u2 = buscar_usuario(nombre);
            if (u2 != NULL) {
                u2->conectado = 0;
                u2->ip[0]     = '\0';
                u2->puerto[0] = '\0';
                m->siguiente  = u2->mensajes;
                u2->mensajes  = m;
            } else {
                free(m);
            }
            pthread_mutex_unlock(&mutex);

            /* Liberar el resto de mensajes no entregados */
            m = sig;
            while (m != NULL) {
                sig = m->siguiente;
                free(m);
                m = sig;
            }
            break;
        }
        free(m);
        m = sig;
    }
}

/*
 * op_disconnect: desconecta un usuario del sistema.
 * Protocolo seccion 8.4.
 * Respuesta: 0 exito, 1 no existe, 2 no conectado, 3 error.
 */
static void op_disconnect(int fd) {
    char nombre[MAX_NAME];
    if (recibir_cadena(fd, nombre, sizeof(nombre)) < 0) {
        enviar_byte(fd, 3);
        return;
    }

    rpc_log(nombre, "DISCONNECT", NULL);

    pthread_mutex_lock(&mutex);

    Usuario *u = buscar_usuario(nombre);
    if (u == NULL) {
        pthread_mutex_unlock(&mutex);
        printf("s> DISCONNECT %s FAIL\n", nombre);
        enviar_byte(fd, 1); /* no existe */
        return;
    }
    if (!u->conectado) {
        pthread_mutex_unlock(&mutex);
        printf("s> DISCONNECT %s FAIL\n", nombre);
        enviar_byte(fd, 2); /* no estaba conectado */
        return;
    }

    u->conectado  = 0;
    u->ip[0]      = '\0';
    u->puerto[0]  = '\0';

    pthread_mutex_unlock(&mutex);
    printf("s> DISCONNECT %s OK\n", nombre);
    enviar_byte(fd, 0);
}

/*
 * op_send: envia un mensaje de texto (sin fichero adjunto) de un usuario a otro.
 * Protocolo secciones 8.5 y 8.6 de la parte 1.
 * Respuesta: 0 + id_str exito, 1 destinatario no existe, 2 error.
 */
static void op_send(int fd) {
    char remitente[MAX_NAME];
    char destinatario[MAX_NAME];
    char texto[MAX_MSG];

    if (recibir_cadena(fd, remitente,    sizeof(remitente))    < 0 ||
        recibir_cadena(fd, destinatario, sizeof(destinatario)) < 0 ||
        recibir_cadena(fd, texto,        sizeof(texto))        < 0) {
        enviar_byte(fd, 2);
        return;
    }

    rpc_log(remitente, "SEND", NULL);

    pthread_mutex_lock(&mutex);

    Usuario *dest = buscar_usuario(destinatario);
    if (dest == NULL) {
        pthread_mutex_unlock(&mutex);
        enviar_byte(fd, 1); /* destinatario no existe */
        return;
    }

    /* Almacenar mensaje sin fichero adjunto */
    unsigned int id = agregar_mensaje(dest, remitente, texto, NULL);
    if (id == 0) {
        pthread_mutex_unlock(&mutex);
        enviar_byte(fd, 2);
        return;
    }

    /* Copiar datos del destinatario para usar fuera del mutex */
    int  dest_conectado = dest->conectado;
    char dest_ip[MAX_IP], dest_puerto[MAX_PORT];
    strncpy(dest_ip,     dest->ip,     MAX_IP   - 1);
    strncpy(dest_puerto, dest->puerto, MAX_PORT - 1);
    dest_ip[MAX_IP - 1]       = '\0';
    dest_puerto[MAX_PORT - 1] = '\0';

    /* Copiar datos del remitente para la notificacion ACK */
    Usuario *rem = buscar_usuario(remitente);
    int  rem_conectado = (rem != NULL) ? rem->conectado : 0;
    char rem_ip[MAX_IP], rem_puerto[MAX_PORT];
    rem_ip[0] = '\0'; rem_puerto[0] = '\0';
    if (rem != NULL) {
        strncpy(rem_ip,     rem->ip,     MAX_IP   - 1);
        strncpy(rem_puerto, rem->puerto, MAX_PORT - 1);
        rem_ip[MAX_IP - 1]       = '\0';
        rem_puerto[MAX_PORT - 1] = '\0';
    }

    pthread_mutex_unlock(&mutex);

    /* Responder al remitente con codigo 0 e id del mensaje */
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%u", id);
    enviar_byte(fd, 0);
    enviar_cadena(fd, id_str);

    /* Si el destinatario esta conectado, intentar entrega inmediata */
    if (dest_conectado) {
        printf("s> SEND MESSAGE %u FROM %s TO %s\n",
               id, remitente, destinatario);

        if (entregar_mensaje(dest_ip, dest_puerto,
                             remitente, id, texto, NULL) == 0) {
            /* Entrega exitosa: eliminar mensaje de la lista de pendientes */
            pthread_mutex_lock(&mutex);
            Usuario *d = buscar_usuario(destinatario);
            if (d != NULL) {
                Mensaje *prev = NULL, *m = d->mensajes;
                while (m != NULL) {
                    if (m->id == id) {
                        if (prev == NULL) d->mensajes    = m->siguiente;
                        else              prev->siguiente = m->siguiente;
                        free(m);
                        break;
                    }
                    prev = m;
                    m    = m->siguiente;
                }
            }
            pthread_mutex_unlock(&mutex);

            /* Notificar al remitente si esta conectado */
            if (rem_conectado) {
                notificar_remitente(rem_ip, rem_puerto, id, NULL);
            }
        } else {
            /* Fallo al entregar: marcar destinatario como desconectado */
            pthread_mutex_lock(&mutex);
            Usuario *d = buscar_usuario(destinatario);
            if (d != NULL) {
                d->conectado = 0;
                d->ip[0]     = '\0';
                d->puerto[0] = '\0';
            }
            pthread_mutex_unlock(&mutex);
        }
    } else {
        printf("s> MESSAGE %u FROM %s TO %s STORED\n",
               id, remitente, destinatario);
    }
}

/*
 * op_sendattach: envia un mensaje con fichero adjunto de un usuario a otro.
 * Protocolo seccion 2.2 del enunciado de la parte 2.
 * Respuesta: 0 + id_str exito, 1 destinatario no existe, 2 error.
 */
static void op_sendattach(int fd) {
    char remitente[MAX_NAME];
    char destinatario[MAX_NAME];
    char texto[MAX_MSG];
    char fichero[MAX_FILE];

    if (recibir_cadena(fd, remitente,    sizeof(remitente))    < 0 ||
        recibir_cadena(fd, destinatario, sizeof(destinatario)) < 0 ||
        recibir_cadena(fd, texto,        sizeof(texto))        < 0 ||
        recibir_cadena(fd, fichero,      sizeof(fichero))      < 0) {
        enviar_byte(fd, 2);
        return;
    }

    rpc_log(remitente, "SENDATTACH", fichero);

    pthread_mutex_lock(&mutex);

    Usuario *dest = buscar_usuario(destinatario);
    if (dest == NULL) {
        pthread_mutex_unlock(&mutex);
        enviar_byte(fd, 1); /* destinatario no existe */
        return;
    }

    /* Almacenar mensaje con nombre de fichero adjunto */
    unsigned int id = agregar_mensaje(dest, remitente, texto, fichero);
    if (id == 0) {
        pthread_mutex_unlock(&mutex);
        enviar_byte(fd, 2);
        return;
    }

    /* Copiar datos del destinatario para usar fuera del mutex */
    int  dest_conectado = dest->conectado;
    char dest_ip[MAX_IP], dest_puerto[MAX_PORT];
    strncpy(dest_ip,     dest->ip,     MAX_IP   - 1);
    strncpy(dest_puerto, dest->puerto, MAX_PORT - 1);
    dest_ip[MAX_IP - 1]       = '\0';
    dest_puerto[MAX_PORT - 1] = '\0';

    /* Copiar datos del remitente para la notificacion ACK */
    Usuario *rem = buscar_usuario(remitente);
    int  rem_conectado = (rem != NULL) ? rem->conectado : 0;
    char rem_ip[MAX_IP], rem_puerto[MAX_PORT];
    rem_ip[0] = '\0'; rem_puerto[0] = '\0';
    if (rem != NULL) {
        strncpy(rem_ip,     rem->ip,     MAX_IP   - 1);
        strncpy(rem_puerto, rem->puerto, MAX_PORT - 1);
        rem_ip[MAX_IP - 1]       = '\0';
        rem_puerto[MAX_PORT - 1] = '\0';
    }

    pthread_mutex_unlock(&mutex);

    /* Responder al remitente con codigo 0 e id del mensaje */
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%u", id);
    enviar_byte(fd, 0);
    enviar_cadena(fd, id_str);

    /* Si el destinatario esta conectado, intentar entrega inmediata */
    if (dest_conectado) {
        printf("s> SEND MESSAGE %u FROM %s TO %s\n",
               id, remitente, destinatario);

        if (entregar_mensaje(dest_ip, dest_puerto,
                             remitente, id, texto, fichero) == 0) {
            /* Entrega exitosa: eliminar mensaje de la lista de pendientes */
            pthread_mutex_lock(&mutex);
            Usuario *d = buscar_usuario(destinatario);
            if (d != NULL) {
                Mensaje *prev = NULL, *m = d->mensajes;
                while (m != NULL) {
                    if (m->id == id) {
                        if (prev == NULL) d->mensajes    = m->siguiente;
                        else              prev->siguiente = m->siguiente;
                        free(m);
                        break;
                    }
                    prev = m;
                    m    = m->siguiente;
                }
            }
            pthread_mutex_unlock(&mutex);

            /* Notificar al remitente con el nombre del fichero si esta conectado */
            if (rem_conectado) {
                notificar_remitente(rem_ip, rem_puerto, id, fichero);
            }
        } else {
            /* Fallo al entregar: marcar destinatario como desconectado */
            pthread_mutex_lock(&mutex);
            Usuario *d = buscar_usuario(destinatario);
            if (d != NULL) {
                d->conectado = 0;
                d->ip[0]     = '\0';
                d->puerto[0] = '\0';
            }
            pthread_mutex_unlock(&mutex);
        }
    } else {
        printf("s> MESSAGE %u FROM %s TO %s STORED\n",
               id, remitente, destinatario);
    }
}

/*
 * op_users: devuelve la lista de usuarios conectados con su IP y puerto.
 * Protocolo seccion 2.4 del enunciado de la parte 2.
 * Cada entrada tiene el formato "nombre :: IP :: puerto".
 * Respuesta: 0 + count + entradas, 1 usuario no conectado, 2 error.
 */
static void op_users(int fd) {
    char nombre[MAX_NAME];
    if (recibir_cadena(fd, nombre, sizeof(nombre)) < 0) {
        enviar_byte(fd, 2);
        return;
    }

    rpc_log(nombre, "USERS", NULL);

    pthread_mutex_lock(&mutex);

    Usuario *sol = buscar_usuario(nombre);
    if (sol == NULL || !sol->conectado) {
        pthread_mutex_unlock(&mutex);
        printf("s> CONNECTEDUSERS FAIL\n");
        enviar_byte(fd, 1);
        return;
    }

    /* Copiar entradas "nombre :: IP :: puerto" de los usuarios conectados */
    int  count = 0;
    char entradas[1024][MAX_NAME + MAX_IP + MAX_PORT + 10];
    Usuario *u = lista_usuarios;
    while (u != NULL && count < 1024) {
        if (u->conectado) {
            snprintf(entradas[count], sizeof(entradas[count]),
                     "%s :: %s :: %s", u->nombre, u->ip, u->puerto);
            count++;
        }
        u = u->siguiente;
    }

    pthread_mutex_unlock(&mutex);

    printf("s> CONNECTEDUSERS OK\n");
    enviar_byte(fd, 0);

    char count_str[16];
    snprintf(count_str, sizeof(count_str), "%d", count);
    enviar_cadena(fd, count_str);

    for (int i = 0; i < count; i++) {
        enviar_cadena(fd, entradas[i]);
    }
}

/* ------------------------------------------------------------------ */
/* Hilo por conexion                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    int  fd;
    char ip[MAX_IP];
} DatosHilo;

/*
 * atender_cliente: funcion ejecutada por cada hilo.
 * Lee la operacion y llama a la funcion correspondiente.
 */
void *atender_cliente(void *arg) {
    DatosHilo *datos = (DatosHilo *)arg;
    int  fd = datos->fd;
    char ip[MAX_IP];
    strncpy(ip, datos->ip, MAX_IP - 1);
    ip[MAX_IP - 1] = '\0';
    free(datos);

    char operacion[32];
    if (recibir_cadena(fd, operacion, sizeof(operacion)) < 0) {
        close(fd);
        return NULL;
    }

    if      (strcmp(operacion, "REGISTER")   == 0) op_register(fd, ip);
    else if (strcmp(operacion, "UNREGISTER") == 0) op_unregister(fd);
    else if (strcmp(operacion, "CONNECT")    == 0) op_connect(fd, ip);
    else if (strcmp(operacion, "DISCONNECT") == 0) op_disconnect(fd);
    else if (strcmp(operacion, "SEND")       == 0) op_send(fd);
    else if (strcmp(operacion, "SENDATTACH") == 0) op_sendattach(fd);
    else if (strcmp(operacion, "USERS")      == 0) op_users(fd);
    else {
        printf("s> Operacion desconocida: %s\n", operacion);
        enviar_byte(fd, 2);
    }

    close(fd);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Manejador de senales                                               */
/* ------------------------------------------------------------------ */

void manejador_senal(int sig) {
    (void)sig;
    printf("\ns> Servidor terminando...\n");
    if (servidor_fd != -1) close(servidor_fd);
    exit(0);
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    int puerto = -1;

    /* Parsear argumento -p <puerto> */
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            puerto = atoi(argv[i + 1]);
        }
    }

    if (puerto <= 0) {
        fprintf(stderr, "Uso: %s -p <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Conectar al servidor RPC de log si LOG_RPC_IP esta definida */
    const char *rpc_ip = getenv("LOG_RPC_IP");
    if (rpc_ip != NULL) {
        rpc_cliente = clnt_create(rpc_ip, LOG_PROG, LOG_VERS, "tcp");
        if (rpc_cliente == NULL) {
            fprintf(stderr, "s> Advertencia: no se pudo conectar al servidor RPC en %s\n",
                    rpc_ip);
        } else {
            printf("s> Conectado al servidor RPC en %s\n", rpc_ip);
        }
    }

    /* Configurar senales */
    signal(SIGINT,  manejador_senal);
    signal(SIGTERM, manejador_senal);

    /* Crear socket del servidor */
    servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_fd < 0) {
        perror("s> Error al crear socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(puerto);

    if (bind(servidor_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("s> Error en bind");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(servidor_fd, BACKLOG) < 0) {
        perror("s> Error en listen");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    printf("s> init server %s:%d\n", "127.0.0.1", puerto);
    printf("s> \n");

    /* Bucle principal: aceptar conexiones y crear hilos detached */
    while (1) {
        struct sockaddr_in ca;
        socklen_t cl = sizeof(ca);

        DatosHilo *datos = (DatosHilo *)malloc(sizeof(DatosHilo));
        if (datos == NULL) continue;

        datos->fd = accept(servidor_fd, (struct sockaddr *)&ca, &cl);
        if (datos->fd < 0) {
            free(datos);
            continue;
        }

        inet_ntop(AF_INET, &ca.sin_addr, datos->ip, sizeof(datos->ip));

        pthread_t      hilo;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&hilo, &attr, atender_cliente, datos) != 0) {
            perror("s> Error al crear hilo");
            close(datos->fd);
            free(datos);
        }

        pthread_attr_destroy(&attr);
    }

    return 0;
}

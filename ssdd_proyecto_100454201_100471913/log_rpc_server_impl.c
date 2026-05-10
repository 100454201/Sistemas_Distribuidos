/*
 * log_rpc_server_impl.c - Implementacion del servidor ONC-RPC de log.
 *
 * Imprime por pantalla cada operacion registrada por el servidor de mensajeria.
 * Para SENDATTACH tambien se muestra el nombre del fichero adjunto.
 * Compilar junto con los ficheros generados por rpcgen (ver Makefile).
 */

#include "log_rpc.h"
#include <stdio.h>

/*
 * log_operacion_1_svc: procedimiento RPC que imprime la operacion recibida.
 * Formato de salida:
 *   usuario    OPERACION            (operaciones sin fichero)
 *   usuario    SENDATTACH fichero   (SENDATTACH incluye el fichero)
 */
int *log_operacion_1_svc(LogPeticion *peticion, struct svc_req *rqstp) {
    static int resultado = 0;
    (void)rqstp; /* no se usa el contexto de la peticion */

    if (peticion->fichero != NULL && peticion->fichero[0] != '\0') {
        /* SENDATTACH: mostrar tambien el nombre del fichero */
        printf("%s\t%s\t%s\n",
               peticion->usuario,
               peticion->operacion,
               peticion->fichero);
    } else {
        printf("%s\t%s\n",
               peticion->usuario,
               peticion->operacion);
    }
    fflush(stdout);

    return &resultado;
}

/*
 * proxy-rpc.c - Proxy del lado del cliente usando ONC RPC
 * Sistemas Distribuidos - Ejercicio Evaluable 3
 *
 * Implementa las funciones de la API de claves.h realizando
 * llamadas RPC al servidor. La direccion IP del servidor se
 * lee de la variable de entorno IP_TUPLAS.
 *
 * Este codigo se compila en libproxyclaves.so.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clavesRPC.h"
#include "claves.h"

/* ------------------------------------------------------------------ */
/* Funcion auxiliar: crea el cliente RPC                               */
/* ------------------------------------------------------------------ */

/*
 * crear_cliente: lee IP_TUPLAS del entorno y crea un cliente RPC TCP.
 * Devuelve el handle CLIENT* en exito, NULL en error.
 */
static CLIENT *crear_cliente(void) {
    char *ip = getenv("IP_TUPLAS");
    if (ip == NULL) {
        fprintf(stderr, "[Proxy RPC] Variable IP_TUPLAS no definida\n");
        return NULL;
    }

    CLIENT *clnt = clnt_create(ip, CLAVES_PROG, CLAVES_VERS, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(ip);
        return NULL;
    }

    return clnt;
}

/* ------------------------------------------------------------------ */
/* Implementacion de la API                                            */
/* ------------------------------------------------------------------ */

int destroy(void) {
    CLIENT *clnt = crear_cliente();
    if (clnt == NULL) return -2;

    int resultado = -2;
    enum clnt_stat retval = destroy_1(&resultado, clnt);

    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "[Proxy RPC] destroy fallo");
        clnt_destroy(clnt);
        return -2;
    }

    clnt_destroy(clnt);
    return resultado;
}

int set_value(char *key, char *value1, int N_value2,
              float *V_value2, struct Paquete value3) {
    /* Validaciones locales */
    if (key    == NULL || strlen(key)    > 255) return -1;
    if (value1 == NULL || strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32)          return -1;
    if (V_value2 == NULL)                        return -1;

    CLIENT *clnt = crear_cliente();
    if (clnt == NULL) return -2;

    /* Construir el argumento RPC */
    SetModifyArgs args;
    args.key    = key;
    args.value1 = value1;
    args.N_value2 = N_value2;
    args.V_value2.VectorFloat_len = N_value2;
    args.V_value2.VectorFloat_val = V_value2;
    args.value3.x = value3.x;
    args.value3.y = value3.y;
    args.value3.z = value3.z;

    int resultado = -2;
    enum clnt_stat retval = set_value_1(args, &resultado, clnt);

    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "[Proxy RPC] set_value fallo");
        clnt_destroy(clnt);
        return -2;
    }

    clnt_destroy(clnt);
    return resultado;
}

int get_value(char *key, char *value1, int *N_value2,
              float *V_value2, struct Paquete *value3) {
    if (key    == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;
    if (value1 == NULL || N_value2 == NULL)  return -1;
    if (V_value2 == NULL || value3 == NULL)  return -1;

    CLIENT *clnt = crear_cliente();
    if (clnt == NULL) return -2;

    GetValueResult result;
    memset(&result, 0, sizeof(GetValueResult));

    enum clnt_stat retval = get_value_1(key, &result, clnt);

    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "[Proxy RPC] get_value fallo");
        clnt_destroy(clnt);
        return -2;
    }

    int res = result.resultado;

    if (res == 0) {
        strncpy(value1, result.value1, 255);
        value1[255] = '\0';

        *N_value2 = result.N_value2;

        for (int i = 0; i < result.N_value2; i++) {
            V_value2[i] = result.V_value2.VectorFloat_val[i];
        }

        value3->x = result.value3.x;
        value3->y = result.value3.y;
        value3->z = result.value3.z;
    }

    /* Liberar memoria XDR del resultado */
    clnt_freeres(clnt, (xdrproc_t)xdr_GetValueResult, (caddr_t)&result);

    clnt_destroy(clnt);
    return res;
}

int modify_value(char *key, char *value1, int N_value2,
                 float *V_value2, struct Paquete value3) {
    /* Validaciones locales */
    if (key    == NULL || strlen(key)    > 255) return -1;
    if (value1 == NULL || strlen(value1) > 255) return -1;
    if (N_value2 < 1 || N_value2 > 32)          return -1;
    if (V_value2 == NULL)                        return -1;

    CLIENT *clnt = crear_cliente();
    if (clnt == NULL) return -2;

    SetModifyArgs args;
    args.key    = key;
    args.value1 = value1;
    args.N_value2 = N_value2;
    args.V_value2.VectorFloat_len = N_value2;
    args.V_value2.VectorFloat_val = V_value2;
    args.value3.x = value3.x;
    args.value3.y = value3.y;
    args.value3.z = value3.z;

    int resultado = -2;
    enum clnt_stat retval = modify_value_1(args, &resultado, clnt);

    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "[Proxy RPC] modify_value fallo");
        clnt_destroy(clnt);
        return -2;
    }

    clnt_destroy(clnt);
    return resultado;
}

int delete_key(char *key) {
    if (key == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;

    CLIENT *clnt = crear_cliente();
    if (clnt == NULL) return -2;

    int resultado = -2;
    enum clnt_stat retval = delete_key_1(key, &resultado, clnt);

    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "[Proxy RPC] delete_key fallo");
        clnt_destroy(clnt);
        return -2;
    }

    clnt_destroy(clnt);
    return resultado;
}

int exist(char *key) {
    if (key == NULL || strlen(key) == 0 || strlen(key) > 255) return -1;

    CLIENT *clnt = crear_cliente();
    if (clnt == NULL) return -2;

    int resultado = -2;
    enum clnt_stat retval = exist_1(key, &resultado, clnt);

    if (retval != RPC_SUCCESS) {
        clnt_perror(clnt, "[Proxy RPC] exist fallo");
        clnt_destroy(clnt);
        return -2;
    }

    clnt_destroy(clnt);
    return resultado;
}

/*
 * clavesRPC_server.c - Implementacion del servidor RPC
 * Sistemas Distribuidos - Ejercicio Evaluable 3
 */

#include "clavesRPC.h"
#include "claves.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool_t
destroy_1_svc(int *result, struct svc_req *rqstp)
{
    (void)rqstp;
    printf("[Servidor RPC] Procesando DESTROY\n");
    *result = destroy();
    printf("[Servidor RPC] DESTROY resultado: %d\n", *result);
    return TRUE;
}

bool_t
set_value_1_svc(SetModifyArgs arg1, int *result, struct svc_req *rqstp)
{
    (void)rqstp;
    printf("[Servidor RPC] Procesando SET_VALUE (key=%s)\n", arg1.key);

    float *V = arg1.V_value2.VectorFloat_val;
    int    N = arg1.V_value2.VectorFloat_len;

    struct Paquete p;
    p.x = arg1.value3.x;
    p.y = arg1.value3.y;
    p.z = arg1.value3.z;

    *result = set_value(arg1.key, arg1.value1, N, V, p);
    printf("[Servidor RPC] SET_VALUE resultado: %d\n", *result);
    return TRUE;
}

bool_t
get_value_1_svc(char *arg1, GetValueResult *result, struct svc_req *rqstp)
{
    (void)rqstp;
    printf("[Servidor RPC] Procesando GET_VALUE (key=%s)\n", arg1);

    memset(result, 0, sizeof(GetValueResult));

    /* Reservar memoria dinamica — XDR la liberara correctamente */
    char  *value1_buf = (char *)malloc(256);
    float *v_buf      = (float *)malloc(32 * sizeof(float));

    if (value1_buf == NULL || v_buf == NULL) {
        free(value1_buf);
        free(v_buf);
        result->resultado = -1;
        return TRUE;
    }

    int N = 0;
    struct Paquete p = {0, 0, 0};

    result->resultado = get_value(arg1, value1_buf, &N, v_buf, &p);

    if (result->resultado == 0) {
        result->value1                   = value1_buf;
        result->N_value2                 = N;
        result->V_value2.VectorFloat_len = N;
        result->V_value2.VectorFloat_val = v_buf;
        result->value3.x = p.x;
        result->value3.y = p.y;
        result->value3.z = p.z;
    } else {
    /* En caso de error, XDR necesita strings validos (no NULL) */
    value1_buf[0] = '\0';
    result->value1                   = value1_buf;
    result->V_value2.VectorFloat_len = 0;
    result->V_value2.VectorFloat_val = v_buf;
}

    printf("[Servidor RPC] GET_VALUE resultado: %d\n", result->resultado);
    return TRUE;
}

bool_t
modify_value_1_svc(SetModifyArgs arg1, int *result, struct svc_req *rqstp)
{
    (void)rqstp;
    printf("[Servidor RPC] Procesando MODIFY_VALUE (key=%s)\n", arg1.key);

    float *V = arg1.V_value2.VectorFloat_val;
    int    N = arg1.V_value2.VectorFloat_len;

    struct Paquete p;
    p.x = arg1.value3.x;
    p.y = arg1.value3.y;
    p.z = arg1.value3.z;

    *result = modify_value(arg1.key, arg1.value1, N, V, p);
    printf("[Servidor RPC] MODIFY_VALUE resultado: %d\n", *result);
    return TRUE;
}

bool_t
delete_key_1_svc(char *arg1, int *result, struct svc_req *rqstp)
{
    (void)rqstp;
    printf("[Servidor RPC] Procesando DELETE_KEY (key=%s)\n", arg1);
    *result = delete_key(arg1);
    printf("[Servidor RPC] DELETE_KEY resultado: %d\n", *result);
    return TRUE;
}

bool_t
exist_1_svc(char *arg1, int *result, struct svc_req *rqstp)
{
    (void)rqstp;
    printf("[Servidor RPC] Procesando EXIST (key=%s)\n", arg1);
    *result = exist(arg1);
    printf("[Servidor RPC] EXIST resultado: %d\n", *result);
    return TRUE;
}

int
claves_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    (void)transp;
    xdr_free(xdr_result, result);
    return 1;
}

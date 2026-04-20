/*
 * clavesRPC.x - Definicion de la interfaz RPC para el servicio de tuplas
 * Sistemas Distribuidos - Ejercicio Evaluable 3
 *
 * Define los tipos de datos y las operaciones del servicio
 * de tuplas usando ONC RPC / XDR.
 */

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */
const MAX_KEY_LEN    = 256;
const MAX_VALUE1_LEN = 256;
const MAX_N_VALUE2   = 32;

/* ------------------------------------------------------------------ */
/* Tipos de datos                                                       */
/* ------------------------------------------------------------------ */

/* Equivalente a struct Paquete de claves.h */
struct PaqueteRPC {
    int x;
    int y;
    int z;
};

/* Vector de floats con longitud variable (maximo 32 elementos) */
typedef float VectorFloat<MAX_N_VALUE2>;

/* ------------------------------------------------------------------ */
/* Estructuras de argumentos (una por operacion que los necesite)      */
/* ------------------------------------------------------------------ */

/* Argumentos para set_value y modify_value */
struct SetModifyArgs {
    string      key<MAX_KEY_LEN>;
    string      value1<MAX_VALUE1_LEN>;
    int         N_value2;
    VectorFloat V_value2;
    PaqueteRPC  value3;
};

/* ------------------------------------------------------------------ */
/* Estructuras de respuesta                                            */
/* ------------------------------------------------------------------ */

/* Respuesta de get_value: incluye resultado y datos si exito */
struct GetValueResult {
    int         resultado;
    string      value1<MAX_VALUE1_LEN>;
    int         N_value2;
    VectorFloat V_value2;
    PaqueteRPC  value3;
};

/* ------------------------------------------------------------------ */
/* Definicion del programa RPC                                         */
/* ------------------------------------------------------------------ */

program CLAVES_PROG {
    version CLAVES_VERS {

        /* destroy: no recibe argumentos, devuelve int */
        int DESTROY(void) = 1;

        /* set_value: recibe todos los parametros, devuelve int */
        int SET_VALUE(SetModifyArgs) = 2;

        /* get_value: recibe la clave, devuelve struct con datos */
        GetValueResult GET_VALUE(string) = 3;

        /* modify_value: recibe todos los parametros, devuelve int */
        int MODIFY_VALUE(SetModifyArgs) = 4;

        /* delete_key: recibe la clave, devuelve int */
        int DELETE_KEY(string) = 5;

        /* exist: recibe la clave, devuelve int */
        int EXIST(string) = 6;

    } = 1;
} = 0x20000001;

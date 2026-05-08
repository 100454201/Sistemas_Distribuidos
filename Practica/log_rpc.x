/*
 * log_rpc.x - Interfaz ONC-RPC para el servicio de log de operaciones.
 *
 * El servidor de mensajeria actua como cliente RPC y envia a este servicio
 * el nombre del usuario y la operacion realizada. Para SENDATTACH tambien
 * se incluye el nombre del fichero adjunto.
 *
 * Justificacion de la interfaz:
 * Se define una unica estructura LogPeticion con tres cadenas (usuario,
 * operacion, fichero) para cubrir todos los casos con un solo procedimiento.
 * El campo fichero se envia vacio ("") cuando la operacion no es SENDATTACH,
 * evitando asi necesitar procedimientos distintos para cada operacion.
 */

/* Datos enviados al servidor RPC por cada operacion recibida */
struct LogPeticion {
    string usuario<256>;    /* nombre del usuario que realiza la operacion  */
    string operacion<256>;  /* nombre de la operacion (REGISTER, SEND, ...) */
    string fichero<256>;    /* nombre de fichero adjunto; "" si no aplica   */
};

program LOG_PROG {
    version LOG_VERS {
        /* Registra una operacion de usuario; devuelve 0 en exito */
        int log_operacion(LogPeticion) = 1;
    } = 1;
} = 0x30000099;

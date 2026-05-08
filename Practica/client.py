from enum import Enum
import argparse
import socket
import json
import urllib.request


class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _web_port = 8080            # puerto del servicio web de normalizacion local
    _connected_user = None      # usuario actualmente conectado
    _listen_sock    = None      # socket de escucha de mensajes
    _listen_thread  = None      # hilo que escucha mensajes
    _usuarios_conectados = {}   # {nombre: (ip, puerto)} actualizado con USERS

    # ******************** HELPERS *******************

    @staticmethod
    def _enviar_cadena(sock, cadena):
        """Envia una cadena terminada en '\0' por el socket."""
        sock.sendall((cadena + '\0').encode())

    @staticmethod
    def _recibir_cadena(sock):
        """Recibe una cadena terminada en '\0' del socket."""
        data = b''
        while True:
            c = sock.recv(1)
            if not c or c == b'\x00':
                break
            data += c
        return data.decode()

    @staticmethod
    def _recibir_byte(sock):
        """Recibe un byte de resultado del socket."""
        data = sock.recv(1)
        if not data:
            return -1
        return data[0]

    @staticmethod
    def _conectar():
        """Crea y devuelve un socket conectado al servidor."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((client._server, client._port))
        return sock

    @staticmethod
    def _normalizar_mensaje(mensaje):
        """Envia el mensaje al servicio web local para eliminar espacios repetidos.
        Si el servicio no esta disponible, devuelve el mensaje sin modificar."""
        try:
            url   = f'http://localhost:{client._web_port}/normalize'
            datos = json.dumps({'message': mensaje}).encode('utf-8')
            req   = urllib.request.Request(
                url, data=datos,
                headers={'Content-Type': 'application/json'}
            )
            with urllib.request.urlopen(req, timeout=2) as resp:
                resultado = json.loads(resp.read().decode('utf-8'))
                return resultado.get('message', mensaje)
        except Exception:
            return mensaje  # si el servicio web no responde, usar mensaje original

    @staticmethod
    def _hilo_escucha(listen_sock, user):
        """Hilo que escucha mensajes y peticiones de fichero entrantes."""
        while True:
            try:
                conn, _ = listen_sock.accept()
                operacion = client._recibir_cadena(conn)

                if operacion == "SEND_MESSAGE":
                    # Mensaje de texto simple del servidor
                    remitente = client._recibir_cadena(conn)
                    msg_id    = client._recibir_cadena(conn)
                    texto     = client._recibir_cadena(conn)
                    conn.close()
                    print(f"\ns> MESSAGE {msg_id} FROM {remitente}")
                    print(f"  {texto}")
                    print("  END")
                    print("c> ", end='', flush=True)

                elif operacion == "SEND_MESS_ACK":
                    # Confirmacion de entrega de mensaje simple
                    msg_id = client._recibir_cadena(conn)
                    conn.close()
                    print(f"\nc> SEND MESSAGE {msg_id} OK")
                    print("c> ", end='', flush=True)

                elif operacion == "SEND_MESSAGE_ATTACH":
                    # Mensaje con fichero adjunto del servidor
                    remitente = client._recibir_cadena(conn)
                    msg_id    = client._recibir_cadena(conn)
                    texto     = client._recibir_cadena(conn)
                    fichero   = client._recibir_cadena(conn)
                    conn.close()
                    print(f"\nc> MESSAGE {msg_id} FROM {remitente}")
                    print(f"  {texto}")
                    print("  END")
                    print(f"  FILE {fichero}")
                    print("c> ", end='', flush=True)

                elif operacion == "SEND_MESS_ATTACH_ACK":
                    # Confirmacion de entrega de mensaje con fichero adjunto
                    msg_id  = client._recibir_cadena(conn)
                    fichero = client._recibir_cadena(conn)
                    conn.close()
                    print(f"\nc> SENDATTACH MESSAGE {msg_id} {fichero} OK")
                    print("c> ", end='', flush=True)

                elif operacion == "GET_FILE":
                    # Peticion de fichero de otro cliente (transferencia P2P)
                    _solicitante = client._recibir_cadena(conn)
                    nombre_fich  = client._recibir_cadena(conn)
                    try:
                        with open(nombre_fich, 'rb') as f:
                            datos = f.read()
                        conn.sendall(datos)
                    except Exception:
                        pass  # si el fichero no existe, cerramos sin enviar nada
                    conn.close()

                else:
                    conn.close()

            except Exception:
                break

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def register(user):
        try:
            sock = client._conectar()
            client._enviar_cadena(sock, "REGISTER")
            client._enviar_cadena(sock, user)
            resp = client._recibir_byte(sock)
            sock.close()

            if resp == 0:
                print("c> REGISTER OK")
                return client.RC.OK
            elif resp == 1:
                print("c> USERNAME IN USE")
                return client.RC.USER_ERROR
            else:
                print("c> REGISTER FAIL")
                return client.RC.ERROR

        except Exception:
            print("c> REGISTER FAIL")
            return client.RC.ERROR

    # *
    # * @param user - User name to unregister from the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def unregister(user) :
        try:
            sock = client._conectar()
            client._enviar_cadena(sock, "UNREGISTER")
            client._enviar_cadena(sock, user)
            resp = client._recibir_byte(sock)
            sock.close()

            if resp == 0:
                print("c> UNREGISTER OK")
                return client.RC.OK
            elif resp == 1:
                print("c> USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                print("c> UNREGISTER FAIL")
                return client.RC.ERROR

        except Exception:
            print("c> UNREGISTER FAIL")
            return client.RC.ERROR

    # *
    # * @param user - User name to connect to the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def connect(user):
        try:
            # Crear socket de escucha en puerto libre asignado por el SO
            listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listen_sock.bind(('', 0))
            puerto_escucha = listen_sock.getsockname()[1]
            listen_sock.listen(10)

            sock = client._conectar()
            client._enviar_cadena(sock, "CONNECT")
            client._enviar_cadena(sock, user)
            client._enviar_cadena(sock, str(puerto_escucha))
            resp = client._recibir_byte(sock)
            sock.close()

            if resp == 0:
                client._connected_user = user
                client._listen_sock    = listen_sock

                import threading
                client._listen_thread = threading.Thread(
                    target=client._hilo_escucha,
                    args=(listen_sock, user),
                    daemon=True
                )
                client._listen_thread.start()

                print("c> CONNECT OK")
                return client.RC.OK

            elif resp == 1:
                listen_sock.close()
                print("c> CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif resp == 2:
                listen_sock.close()
                print("c> USER ALREADY CONNECTED")
                return client.RC.USER_ERROR
            else:
                listen_sock.close()
                print("c> CONNECT FAIL")
                return client.RC.ERROR

        except Exception:
            print("c> CONNECT FAIL")
            return client.RC.ERROR

    # *
    # * @param user - User name to disconnect from the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or is not connected
    # * @return ERROR if another error occurred
    @staticmethod
    def disconnect(user):
        try:
            sock = client._conectar()
            client._enviar_cadena(sock, "DISCONNECT")
            client._enviar_cadena(sock, user)
            resp = client._recibir_byte(sock)
            sock.close()

            if resp == 0:
                if client._listen_sock is not None:
                    client._listen_sock.close()
                    client._listen_sock   = None
                    client._listen_thread = None
                client._connected_user      = None
                client._usuarios_conectados = {}
                print("c> DISCONNECT OK")
                return client.RC.OK

            elif resp == 1:
                print("c> DISCONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif resp == 2:
                print("c> DISCONNECT FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            else:
                print("c> DISCONNECT FAIL")
                return client.RC.ERROR

        except Exception:
            if client._listen_sock is not None:
                client._listen_sock.close()
                client._listen_sock   = None
                client._listen_thread = None
            client._connected_user      = None
            client._usuarios_conectados = {}
            print("c> DISCONNECT FAIL")
            return client.RC.ERROR

    # *
    # * @return OK if successful
    # * @return USER_ERROR if the requesting user is not connected
    # * @return ERROR if another error occurred
    @staticmethod
    def users() :
        try:
            sock = client._conectar()
            client._enviar_cadena(sock, "USERS")
            client._enviar_cadena(sock, client._connected_user)
            resp = client._recibir_byte(sock)

            if resp == 0:
                count    = int(client._recibir_cadena(sock))
                nueva_tabla = {}
                entradas = []
                for _ in range(count):
                    entrada = client._recibir_cadena(sock)
                    entradas.append(entrada)
                    # Formato del servidor: "nombre :: IP :: puerto"
                    partes = entrada.split(' :: ')
                    if len(partes) == 3:
                        nueva_tabla[partes[0]] = (partes[1], partes[2])
                sock.close()
                client._usuarios_conectados = nueva_tabla
                print(f"c> CONNECTED USERS ({count} users connected) OK")
                for entrada in entradas:
                    print(f"  {entrada}")
                return client.RC.OK

            elif resp == 1:
                sock.close()
                print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
                return client.RC.USER_ERROR
            else:
                sock.close()
                print("c> CONNECTED USERS FAIL")
                return client.RC.ERROR

        except Exception:
            print("c> CONNECTED USERS FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (message queued)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def send(user, message) :
        try:
            # Normalizar el mensaje a traves del servicio web local
            message = client._normalizar_mensaje(message)
            sock = client._conectar()
            client._enviar_cadena(sock, "SEND")
            client._enviar_cadena(sock, client._connected_user)
            client._enviar_cadena(sock, user)
            client._enviar_cadena(sock, message)
            resp = client._recibir_byte(sock)

            if resp == 0:
                msg_id = client._recibir_cadena(sock)
                sock.close()
                print(f"c> SEND OK - MESSAGE {msg_id}")
                return client.RC.OK
            elif resp == 1:
                sock.close()
                print("c> SEND FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                sock.close()
                print("c> SEND FAIL")
                return client.RC.ERROR

        except Exception:
            print("c> SEND FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - File name to attach
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (message queued)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def sendAttach(user, file, message) :
        try:
            # Normalizar el mensaje a traves del servicio web local
            message = client._normalizar_mensaje(message)
            sock = client._conectar()
            client._enviar_cadena(sock, "SENDATTACH")
            client._enviar_cadena(sock, client._connected_user)
            client._enviar_cadena(sock, user)
            client._enviar_cadena(sock, message)
            client._enviar_cadena(sock, file)
            resp = client._recibir_byte(sock)

            if resp == 0:
                msg_id = client._recibir_cadena(sock)
                sock.close()
                print(f"c> SENDATTACH OK - MESSAGE {msg_id}")
                return client.RC.OK
            elif resp == 1:
                sock.close()
                print("c> SENDATTACH FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                sock.close()
                print("c> SENDATTACH FAIL")
                return client.RC.ERROR

        except Exception:
            print("c> SENDATTACH FAIL")
            return client.RC.ERROR

    # *
    # * @param user          - Owner of the remote file
    # * @param fileName      - Remote file name to transfer
    # * @param localFileName - Local path where the file will be saved
    # *
    # * @return OK if the file was transferred successfully
    # * @return USER_ERROR if the user is not connected
    # * @return ERROR if another error occurred
    @staticmethod
    def getfile(user, fileName, localFileName):
        try:
            # Buscar IP y puerto del usuario; refrescar si no esta en la tabla
            if user not in client._usuarios_conectados:
                client.users()
            if user not in client._usuarios_conectados:
                print("c> FILE TRANSFER FAILED, user not connected.")
                return client.RC.USER_ERROR

            ip, puerto = client._usuarios_conectados[user]

            # Conectar directamente al hilo de escucha del cliente remoto
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((ip, int(puerto)))

            client._enviar_cadena(sock, "GET_FILE")
            client._enviar_cadena(sock, client._connected_user)
            client._enviar_cadena(sock, fileName)

            # Recibir el contenido del fichero y guardarlo localmente
            contenido = b''
            while True:
                datos = sock.recv(4096)
                if not datos:
                    break
                contenido += datos
            sock.close()

            with open(localFileName, 'wb') as f:
                f.write(contenido)

            print("c> FILE TRANSFER OK")
            return client.RC.OK

        except Exception:
            print("c> FILE TRANSFER FAILED")
            return client.RC.ERROR

    # *
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="USERS") :
                        if (len(line) == 1) :
                            client.users()
                        else :
                            print("Syntax error. Usage: USERS")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            message = ' '.join(line[3:])
                            client.sendAttach(line[1], line[2], message)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <filename> <message>")

                    elif(line[0]=="GETFILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GETFILE <userName> <fileName> <localFileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")

    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        parser.add_argument('-w', type=int, default=8080,  help='Web service port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")
            return False

        client._server   = args.s
        client._port     = args.p
        client._web_port = args.w

        return True

    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        client.shell()
        print("+++ FINISHED +++")


if __name__=="__main__":
    client.main([])

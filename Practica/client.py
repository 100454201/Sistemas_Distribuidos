from enum import Enum
import argparse
import socket


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
    _connected_user = None      # usuario actualmente conectado
    _listen_sock    = None      # socket de escucha de mensajes
    _listen_thread  = None      # hilo que escucha mensajes

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
    def _hilo_escucha(listen_sock, user):
        """Hilo que escucha mensajes entrantes del servidor."""
        while True:
            try:
                conn, _ = listen_sock.accept()
                operacion = client._recibir_cadena(conn)

                if operacion == "SEND_MESSAGE":
                    remitente = client._recibir_cadena(conn)
                    msg_id    = client._recibir_cadena(conn)
                    texto     = client._recibir_cadena(conn)
                    conn.close()
                    print(f"\ns> MESSAGE {msg_id} FROM {remitente}")
                    print(f"  {texto}")
                    print("  END")
                    print("c> ", end='', flush=True)

                elif operacion == "SEND_MESS_ACK":
                    msg_id = client._recibir_cadena(conn)
                    conn.close()
                    print(f"\nc> SEND MESSAGE {msg_id} OK")
                    print("c> ", end='', flush=True)

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

        except Exception as e:
            print("c> REGISTER FAIL")
            return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 * 
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user) :
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

        except Exception as e:
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
            # 1. Buscar puerto libre
            listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listen_sock.bind(('', 0))  # puerto 0 = el SO asigna uno libre
            puerto_escucha = listen_sock.getsockname()[1]
            listen_sock.listen(10)

            # 2. Enviar solicitud de conexion al servidor
            sock = client._conectar()
            client._enviar_cadena(sock, "CONNECT")
            client._enviar_cadena(sock, user)
            client._enviar_cadena(sock, str(puerto_escucha))
            resp = client._recibir_byte(sock)
            sock.close()

            if resp == 0:
                # 3. Guardar estado y arrancar hilo de escucha
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

        except Exception as e:
            print("c> CONNECT FAIL")
            return client.RC.ERROR

    # *
    # * @param user - User name to connect to the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users() :
        try:
            sock = client._conectar()
            client._enviar_cadena(sock, "USERS")
            client._enviar_cadena(sock, client._connected_user)
            resp = client._recibir_byte(sock)

            if resp == 0:
                count = int(client._recibir_cadena(sock))
                nombres = []
                for _ in range(count):
                    nombres.append(client._recibir_cadena(sock))
                sock.close()
                print(f"c> CONNECTED USERS ({count} users connected) OK")
                for nombre in nombres:
                    print(f"  {nombre}")
                return client.RC.OK

            elif resp == 1:
                sock.close()
                print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
                return client.RC.USER_ERROR
            else:
                sock.close()
                print("c> CONNECTED USERS FAIL")
                return client.RC.ERROR

        except Exception as e:
            print("c> CONNECTED USERS FAIL")
            return client.RC.ERROR


    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
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
                # Parar el hilo de escucha cerrando el socket
                if client._listen_sock is not None:
                    client._listen_sock.close()
                    client._listen_sock   = None
                    client._listen_thread = None
                client._connected_user = None
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

        except Exception as e:
            # Aunque falle, parar el hilo igualmente
            if client._listen_sock is not None:
                client._listen_sock.close()
                client._listen_sock   = None
                client._listen_thread = None
            client._connected_user = None
            print("c> DISCONNECT FAIL")
            return client.RC.ERROR
    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  send(user,  message) :
        try:
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

        except Exception as e:
            print("c> SEND FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user,  file,  message) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
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
                            print("Syntax error. Usage: CONNECTED_USERS <userName>")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            #  Remove first two words
                            message = ' '.join(line[3:])
                            client.sendAttach(line[1], line[2], message)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <filename> <message>")

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
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])

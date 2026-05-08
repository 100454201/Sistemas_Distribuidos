"""
Servicio web de normalizacion de mensajes.
Elimina los espacios en blanco repetidos de los mensajes enviados por los clientes,
de forma que las palabras queden separadas por un unico espacio.
Uso: python3 web_service.py -p <puerto>
"""

import re
import json
import argparse
from http.server import HTTPServer, BaseHTTPRequestHandler


class NormalizadorHandler(BaseHTTPRequestHandler):
    """Manejador HTTP que expone el endpoint POST /normalize."""

    def log_message(self, format, *args):
        """Sobrescribe el log por defecto para mostrar solo peticiones relevantes."""
        print(f"ws> {self.address_string()} - {format % args}")

    def do_POST(self):
        """Atiende peticiones POST en /normalize."""
        if self.path != '/normalize':
            self._responder(404, {'error': 'Not found'})
            return

        # Leer el cuerpo de la peticion
        longitud = int(self.headers.get('Content-Length', 0))
        cuerpo   = self.rfile.read(longitud)

        try:
            datos   = json.loads(cuerpo.decode('utf-8'))
            mensaje = datos.get('message', '')

            # Normalizar: colapsar multiples espacios en uno y eliminar extremos
            normalizado = re.sub(r' +', ' ', mensaje).strip()

            print(f"ws> NORMALIZE '{mensaje}' -> '{normalizado}'")
            self._responder(200, {'message': normalizado})

        except Exception:
            self._responder(400, {'error': 'Bad request'})

    def _responder(self, codigo, datos):
        """Envia una respuesta JSON con el codigo HTTP indicado."""
        cuerpo = json.dumps(datos).encode('utf-8')
        self.send_response(codigo)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(cuerpo)))
        self.end_headers()
        self.wfile.write(cuerpo)


def main():
    parser = argparse.ArgumentParser(description='Servicio web de normalizacion')
    parser.add_argument('-p', type=int, default=8080, help='Puerto de escucha')
    args = parser.parse_args()

    servidor = HTTPServer(('0.0.0.0', args.p), NormalizadorHandler)
    print(f"ws> Servicio web iniciado en puerto {args.p}")
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        print("\nws> Servicio web terminando...")
        servidor.server_close()


if __name__ == '__main__':
    main()

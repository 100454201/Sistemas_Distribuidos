# Ejercicio Evaluable 1 - Parte A (No Distribuida)
## Sistemas Distribuidos - Curso 2025-2026

### Archivos implementados

- **claves.h**: API del servicio (proporcionado)
- **claves.c**: Implementación del servicio con lista enlazada dinámica
- **app-cliente.c**: Aplicación cliente con plan de pruebas exhaustivo
- **Makefile**: Para compilar la biblioteca y el ejecutable

### Características de la implementación

#### claves.c
- **Estructura de datos**: Lista enlazada dinámica (sin límite de elementos)
- **Concurrencia**: Mutex POSIX para operaciones atómicas
- **Validaciones**: 
  - Claves y value1 máximo 255 caracteres
  - N_value2 entre 1 y 32
  - Detección de claves duplicadas
  - Manejo robusto de errores

#### Funciones implementadas
1. `destroy()` - Inicializa/limpia el servicio
2. `set_value()` - Inserta nueva tupla
3. `get_value()` - Recupera valores por clave
4. `modify_value()` - Modifica tupla existente
5. `delete_key()` - Elimina tupla
6. `exist()` - Verifica existencia de clave

#### Plan de pruebas (app-cliente.c)
El cliente implementa 15 baterías de pruebas:

1. Inicialización del servicio
2. Inserción de tuplas básicas
3. Error: clave duplicada
4. Error: N_value2 fuera de rango (0, 33)
5. Verificación de existencia
6. Recuperación de valores
7. Error: get_value con clave inexistente
8. Modificación de valores
9. Error: modify_value con clave inexistente
10. Eliminación de claves
11. Error: delete_key con clave inexistente
12. Límites: cadenas >255 caracteres
13. Vector de tamaño máximo (32 elementos)
14. Múltiples inserciones (10 tuplas)
15. Destrucción del servicio con datos

### Compilación y ejecución

#### En Linux / guernika:

```bash
# Compilar todo (genera libclaves.so y app-cliente)
make

# Ejecutar el cliente de pruebas
./app-cliente

# Limpiar archivos generados
make clean
```

#### Estructura de compilación:

El Makefile genera:
- `claves.o`: Objeto compilado con -fPIC
- `libclaves.so`: Biblioteca dinámica compartida
- `app-cliente`: Ejecutable enlazado con libclaves.so

#### Opciones adicionales:

```bash
make local      # Compila solo la versión no distribuida
make help       # Muestra ayuda
```

### Verificación en guernika

Para asegurar que funciona correctamente en guernika:

```bash
# 1. Subir archivos a guernika
scp claves.h claves.c app-cliente.c Makefile usuario@guernika.i2basque.es:~/ejercicio1/

# 2. Conectar y compilar
ssh usuario@guernika.i2basque.es
cd ~/ejercicio1
make

# 3. Ejecutar pruebas
./app-cliente

# 4. Verificar biblioteca
ldd app-cliente              # Mostrar dependencias
ls -lh libclaves.so          # Verificar biblioteca generada
```

### Notas técnicas

- **Thread-safety**: Todas las operaciones protegidas con mutex
- **Memoria dinámica**: malloc/free para nodos de lista
- **Portabilidad**: Código estándar POSIX/C99
- **Sin límites artificiales**: La lista crece dinámicamente
- **Rpath configurado**: No requiere LD_LIBRARY_PATH manual

### Próximos pasos (Parte B)

Para la versión distribuida se necesitará:
- `servidor-mq.c`: Servidor con colas POSIX
- `proxy-mq.c`: Proxy cliente
- `libproxyclaves.so`: Biblioteca del proxy
- Mismo `app-cliente.c` sin modificaciones

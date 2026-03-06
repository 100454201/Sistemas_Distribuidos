# Ejercicio Evaluable 1 - Parte B (Versión Distribuida)
## Sistemas Distribuidos - Curso 2025-2026

### Archivos implementados (Parte B)

- **protocolo.h**: Definiciones de estructuras de mensajes y constantes
- **servidor-mq.c**: Servidor concurrente con colas POSIX
- **proxy-mq.c**: Proxy del lado del cliente
- **Makefile**: Actualizado para compilar ambas versiones

### Arquitectura de la versión distribuida

```
┌─────────────────┐         Cola POSIX          ┌──────────────────┐
│  app-cliente-mq │ ────────────────────────────> │  servidor_mq     │
│                 │   /cola_servidor_sd          │                  │
│ (usa API)       │                              │ (procesa datos)  │
│                 │   /cola_cliente_sd_<PID>     │                  │
│ libproxyclaves  │ <──────────────────────────── │  libclaves.so    │
│     .so         │         Respuesta            │                  │
└─────────────────┘                               └──────────────────┘
```

### Diseño del protocolo de comunicación

#### Colas de mensajes POSIX:
1. **Cola del servidor**: `/cola_servidor_sd` (única, compartida)
   - Todos los clientes envían peticiones aquí
   - El servidor espera en esta cola

2. **Cola del cliente**: `/cola_cliente_sd_<PID>` (una por cliente)
   - Cada cliente crea su propia cola usando su PID
   - El servidor envía respuestas a la cola específica del cliente

#### Estructura de mensajes:

**Petición** (Cliente → Servidor):
```c
- tipo_operacion: OP_DESTROY, OP_SET_VALUE, OP_GET_VALUE, etc.
- pid_cliente: Para identificar la cola de respuesta
- key, value1, N_value2, V_value2, value3: Parámetros
```

**Respuesta** (Servidor → Cliente):
```c
- resultado: 0 (éxito), -1 (error servicio), -2 (error comunicación)
- value1, N_value2, V_value2, value3: Datos de respuesta (si aplica)
```

### Mecanismo de concurrencia del servidor

**Estrategia**: Hilos bajo demanda (thread-per-request)

**Justificación**:
- ✅ Simplicidad de implementación
- ✅ Aislamiento natural entre peticiones
- ✅ Hilos desacoplados (detached) - sin gestión manual
- ✅ Suficiente para carga moderada
- ✅ El servicio de tuplas ya tiene mutex interno

**Flujo**:
1. Servidor espera petición en cola principal
2. Al recibir petición, crea hilo detached
3. Hilo procesa petición → invoca función de libclaves.so
4. Hilo envía respuesta a cola del cliente
5. Hilo termina automáticamente

### Códigos de error

- **0**: Operación exitosa
- **-1**: Error del servicio de tuplas (clave no existe, duplicada, fuera de rango, etc.)
- **-2**: Error de comunicación (cola inexistente, timeout, error al enviar/recibir)

### Compilación

```bash
# Compilar ambas versiones
make

# Solo versión distribuida
make distribuido

# Limpiar
make clean
```

**Archivos generados**:
- `libclaves.so` - Biblioteca del servicio (parte A)
- `libproxyclaves.so` - Biblioteca proxy (parte B)
- `app-cliente` - Cliente local (parte A)
- `servidor_mq` - Servidor distribuido (parte B)
- `app-cliente-mq` - Cliente distribuido (parte B)

### Ejecución

#### Terminal 1 - Servidor:
```bash
./servidor_mq
```

#### Terminal 2 - Cliente:
```bash
./app-cliente-mq
```

#### Terminal 3 - Otro cliente (concurrente):
```bash
./app-cliente-mq
```

### Plan de pruebas (Parte B)

El mismo `app-cliente.c` se usa sin modificaciones. Las 15 baterías de pruebas son:

1. ✅ Inicialización (destroy)
2. ✅ Inserción básica (set_value)
3. ❌ Clave duplicada
4. ❌ N_value2 fuera de rango
5. ✅ Verificación de existencia
6. ✅ Recuperación de valores
7. ❌ get_value con clave inexistente
8. ✅ Modificación de valores
9. ❌ modify_value con clave inexistente
10. ✅ Eliminación de claves
11. ❌ delete_key con clave inexistente
12. ❌ Cadenas >255 caracteres
13. ✅ Vector tamaño máximo (32)
14. ✅ Múltiples inserciones
15. ✅ Destrucción completa

### Pruebas de concurrencia

Para probar el comportamiento concurrente:

```bash
# Terminal 1
./servidor_mq

# Terminal 2, 3, 4... (ejecutar simultáneamente)
./app-cliente-mq &
./app-cliente-mq &
./app-cliente-mq &
wait
```

### Limpieza de colas

Si las colas quedan en estado inconsistente:

```bash
# Ver colas activas
ls -la /dev/mqueue/

# Eliminar colas manualmente
rm /dev/mqueue/cola_servidor_sd
rm /dev/mqueue/cola_cliente_sd_*

# O usar el script de limpieza
./limpiar_colas.sh
```

### Diferencias entre Parte A y Parte B

| Aspecto | Parte A (Local) | Parte B (Distribuida) |
|---------|----------------|---------------------|
| Biblioteca cliente | `libclaves.so` | `libproxyclaves.so` |
| Comunicación | Llamadas directas | Colas POSIX |
| Ejecutable cliente | `app-cliente` | `app-cliente-mq` |
| Servidor | No existe | `servidor_mq` |
| Código cliente | Idéntico `app-cliente.c` | Idéntico `app-cliente.c` |
| Errores | Solo -1 | -1 (servicio), -2 (comunicación) |

### Notas técnicas

- **Thread-safety**: Garantizado por mutex en claves.c
- **Timeouts**: 5 segundos en recepción de respuestas
- **Limpieza automática**: Colas del cliente se eliminan al terminar (destructor)
- **PID único**: Cada cliente usa su PID para identificar su cola
- **Hilos detached**: No requieren pthread_join

### Verificación en guernika

```bash
# 1. Compilar
make

# 2. Verificar bibliotecas
ldd servidor_mq
ldd app-cliente-mq

# 3. Ejecutar servidor en background
./servidor_mq &

# 4. Ejecutar cliente
./app-cliente-mq

# 5. Terminar servidor
killall servidor_mq

# 6. Limpiar colas
rm /dev/mqueue/cola_*
```

### Solución de problemas comunes

**Problema**: `mq_open: No such file or directory`
- **Causa**: El servidor no está ejecutándose
- **Solución**: Ejecutar `./servidor_mq` primero

**Problema**: `mq_open: File exists`
- **Causa**: Cola anterior no se limpió
- **Solución**: `rm /dev/mqueue/cola_cliente_sd_<PID>`

**Problema**: Timeout esperando respuesta
- **Causa**: Servidor caído o sobrecargado
- **Solución**: Reiniciar servidor, verificar logs

**Problema**: Error al crear cola del servidor
- **Causa**: Permisos insuficientes o cola huérfana
- **Solución**: `rm /dev/mqueue/cola_servidor_sd` y reiniciar

# Ejercicio Evaluable 1 - Servicio de Tuplas
## Sistemas Distribuidos - Curso 2025-2026
**Fecha límite**: 15 de marzo de 2026

---

## 📋 Descripción

Implementación de un servicio de almacenamiento de tuplas `<key, value1, value2, value3>` en dos versiones:
- **Parte A**: Versión monolítica (biblioteca dinámica local)
- **Parte B**: Versión distribuida (cliente-servidor con colas POSIX)

---

## 🗂️ Estructura del proyecto

```
ejercicio_evaluable1/
├── claves.h                  # API del servicio (proporcionada)
├── claves.c                  # Implementación del servicio (lista enlazada + mutex)
├── protocolo.h               # Protocolo de comunicación (Parte B)
├── app-cliente.c             # Cliente de pruebas (15 baterías)
├── servidor-mq.c             # Servidor concurrente (Parte B)
├── proxy-mq.c                # Proxy cliente (Parte B)
├── Makefile                  # Compilación automática
├── README.md                 # Este archivo
├── README_PARTE_A.md         # Documentación Parte A
├── README_PARTE_B.md         # Documentación Parte B
├── limpiar_colas.sh          # Script de limpieza
├── test_local.sh             # Script de pruebas Parte A
└── test_distribuido.sh       # Script de pruebas Parte B
```

---

## 🚀 Inicio rápido

### Compilar todo
```bash
make
```

### Probar versión local (Parte A)
```bash
./test_local.sh
# O manualmente:
./app-cliente
```

### Probar versión distribuida (Parte B)
```bash
./test_distribuido.sh
# O manualmente en 2 terminales:
# Terminal 1:
./servidor_mq

# Terminal 2:
./app-cliente-mq
```

---

## 📦 Compilación detallada

```bash
# Compilar ambas versiones
make

# Solo versión local
make local

# Solo versión distribuida
make distribuido

# Limpiar archivos generados
make clean

# Ver ayuda
make help
```

### Archivos generados:

| Archivo | Descripción |
|---------|-------------|
| `libclaves.so` | Biblioteca del servicio (Parte A) |
| `app-cliente` | Cliente local (Parte A) |
| `libproxyclaves.so` | Biblioteca proxy (Parte B) |
| `servidor_mq` | Servidor distribuido (Parte B) |
| `app-cliente-mq` | Cliente distribuido (Parte B) |

---

## 🧪 Plan de pruebas

El archivo `app-cliente.c` implementa **15 baterías de pruebas**:

### Pruebas exitosas ✅
1. Inicialización del servicio (destroy)
2. Inserción de tuplas básicas
5. Verificación de existencia
6. Recuperación de valores
8. Modificación de valores
10. Eliminación de claves
13. Vector de tamaño máximo (32 elementos)
14. Múltiples inserciones (10 tuplas)
15. Destrucción del servicio con datos

### Manejo de errores ❌
3. Clave duplicada (debe fallar)
4. N_value2 fuera de rango (0, 33)
7. get_value con clave inexistente
9. modify_value con clave inexistente
11. delete_key con clave inexistente
12. Cadenas >255 caracteres

---

## 🏗️ Arquitectura

### Parte A (Monolítica)
```
┌─────────────────┐
│  app-cliente    │
│                 │
│  (usa API)      │
│                 │
│  libclaves.so   │
│  (lista + mutex)│
└─────────────────┘
```

### Parte B (Distribuida)
```
┌─────────────────┐         Cola POSIX          ┌──────────────────┐
│  app-cliente-mq │ ────────────────────────────> │  servidor_mq     │
│                 │   /cola_servidor_sd          │                  │
│ (usa API)       │                              │ (hilos demanda)  │
│                 │   /cola_cliente_sd_<PID>     │                  │
│ libproxyclaves  │ <──────────────────────────── │  libclaves.so    │
│     .so         │                               │                  │
└─────────────────┘                               └──────────────────┘
```

---

## 🔧 Características técnicas

### Parte A
- **Estructura de datos**: Lista enlazada dinámica (sin límite)
- **Concurrencia**: Mutex POSIX para operaciones atómicas
- **Validaciones**: Límites de cadenas (255), rangos (1-32), claves duplicadas
- **Manejo de errores**: Retorna -1 en caso de error

### Parte B
- **Comunicación**: Colas de mensajes POSIX
- **Servidor**: Concurrente (hilos bajo demanda)
- **Protocolo**: Petición-respuesta con identificación por PID
- **Códigos de error**:
  - `0`: Éxito
  - `-1`: Error del servicio
  - `-2`: Error de comunicación
- **Timeout**: 5 segundos en recepción de respuestas

---

## 📊 API del servicio

```c
int destroy(void);
int set_value(char *key, char *value1, int N_value2, 
              float *V_value2, struct Paquete value3);
int get_value(char *key, char *value1, int *N_value2, 
              float *V_value2, struct Paquete *value3);
int modify_value(char *key, char *value1, int N_value2, 
                 float *V_value2, struct Paquete value3);
int delete_key(char *key);
int exist(char *key);
```

### Estructura Paquete
```c
struct Paquete {
    int x;
    int y;
    int z;
};
```

---

## 🛠️ Troubleshooting

### Error: "mq_open: No such file or directory"
**Causa**: Servidor no ejecutándose  
**Solución**: Ejecutar `./servidor_mq` primero

### Error: "mq_open: File exists"
**Causa**: Cola anterior no limpiada  
**Solución**: `./limpiar_colas.sh` o `rm /dev/mqueue/cola_*`

### Error: Timeout esperando respuesta
**Causa**: Servidor caído o bloqueado  
**Solución**: Reiniciar servidor, verificar logs

### Compilación falla
**Causa**: Falta librería pthread o rt  
**Solución**: Verificar que `-lpthread -lrt` están en LDFLAGS

---

## 🧹 Limpieza

### Limpiar archivos compilados
```bash
make clean
```

### Limpiar colas de mensajes
```bash
./limpiar_colas.sh
# O manualmente:
rm /dev/mqueue/cola_*
```

---

## ✅ Checklist de entrega

### Código
- [x] claves.h (proporcionado)
- [x] claves.c (implementación)
- [x] app-cliente.c (pruebas)
- [x] servidor-mq.c (servidor)
- [x] proxy-mq.c (proxy cliente)
- [x] protocolo.h (protocolo)
- [x] Makefile (compilación)

### Documentación
- [x] README.md (principal)
- [x] README_PARTE_A.md
- [x] README_PARTE_B.md
- [ ] **MEMORIA.pdf** (máx. 5 páginas) ⚠️ PENDIENTE

### Scripts
- [x] test_local.sh
- [x] test_distribuido.sh
- [x] limpiar_colas.sh

### Verificación
- [ ] Compilar en guernika
- [ ] Ejecutar pruebas en guernika
- [ ] Verificar concurrencia

---

## 📝 Contenido de la memoria (pendiente)

La memoria debe incluir:

1. **Portada** (título, nombre, fecha)
2. **Diseño**:
   - Estructura de datos elegida (lista enlazada) y justificación
   - Mecanismo de concurrencia (mutex + hilos bajo demanda)
3. **Protocolo de comunicación**:
   - Diagrama de flujo o pseudocódigo
   - Descripción de colas y mensajes
4. **Batería de pruebas**:
   - Descripción de las 15 pruebas
   - Cómo ejecutarlas
5. **Compilación y ejecución**:
   - Comandos `make`
   - Ejecución en guernika

**Máximo**: 5 páginas (incluida portada)

---

## 👥 Autor

David Sierra

---

## 📅 Fecha de entrega

**15 de marzo de 2026 - 23:55 horas**

Entrega mediante Aula Global: `ejercicio_evaluable1.zip`

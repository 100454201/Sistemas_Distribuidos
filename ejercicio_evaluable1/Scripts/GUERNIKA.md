# Instrucciones para probar en Guernika
## Ejercicio Evaluable 1 - Sistemas Distribuidos

---

## 📤 Paso 1: Subir archivos a Guernika

Desde tu máquina local (Windows/Linux/Mac):

```bash
# Crear directorio temporal con los archivos
mkdir ejercicio1_temp
cd ejercicio1_temp

# Copiar archivos necesarios (ejecutar desde el directorio del ejercicio)
# O simplemente copiar todo el contenido

# Desde Windows PowerShell:
scp claves.h claves.c app-cliente.c servidor-mq.c proxy-mq.c protocolo.h Makefile usuario@guernika.i2basque.es:~/ejercicio1/

# O si tienes el zip:
scp ejercicio_evaluable1.zip usuario@guernika.i2basque.es:~/
```

---

## 🔐 Paso 2: Conectar a Guernika

```bash
ssh usuario@guernika.i2basque.es
```

---

## 📂 Paso 3: Preparar directorio

```bash
# Si subiste un zip:
cd ~
unzip ejercicio_evaluable1.zip -d ejercicio1
cd ejercicio1

# Si copiaste archivos directamente:
cd ~/ejercicio1

# Dar permisos de ejecución a los scripts
chmod +x *.sh
```

---

## 🔨 Paso 4: Compilar

```bash
# Ver opciones de compilación
make help

# Compilar todo
make

# O por partes:
make local        # Solo Parte A
make distribuido  # Solo Parte B
```

### Verificar que se generaron los archivos:

```bash
ls -lh *.so       # Bibliotecas
ls -lh app-*      # Clientes
ls -lh servidor_* # Servidor
```

Deberías ver:
- `libclaves.so`
- `libproxyclaves.so`
- `app-cliente`
- `app-cliente-mq`
- `servidor_mq`

---

## 🧪 Paso 5: Probar Parte A (versión local)

### Opción 1: Script automático
```bash
./test_local.sh
```

### Opción 2: Manual
```bash
./app-cliente
```

Deberías ver la salida de las 15 pruebas con indicadores de éxito/fallo.

---

## 🧪 Paso 6: Probar Parte B (versión distribuida)

### Opción 1: Script automático
```bash
./test_distribuido.sh
```

### Opción 2: Manual (2 terminales)

#### Terminal 1 - Servidor:
```bash
# Limpiar colas anteriores
./limpiar_colas.sh

# O manualmente:
rm -f /dev/mqueue/cola_* 2>/dev/null

# Iniciar servidor
./servidor_mq
```

Deberías ver:
```
===========================================
  SERVIDOR DE TUPLAS - Colas POSIX
  Sistemas Distribuidos - Ejercicio 1
===========================================

[Servidor] Inicializando servicio de tuplas...
[Servidor] Creando cola: /cola_servidor_sd
[Servidor] Servidor iniciado. Esperando peticiones...
[Servidor] Presione Ctrl+C para terminar.
```

#### Terminal 2 - Cliente:

Abrir nueva conexión SSH a guernika:
```bash
ssh usuario@guernika.i2basque.es
cd ~/ejercicio1

# Ejecutar cliente
./app-cliente-mq
```

Deberías ver el mismo output que en la Parte A, pero ahora comunicándose por colas.

En el Terminal 1 (servidor) verás logs de las peticiones procesadas:
```
[Servidor] Procesando DESTROY
[Servidor] Respuesta enviada (resultado=0)
[Servidor] Procesando SET_VALUE (key=clave1)
[Servidor] Respuesta enviada (resultado=0)
...
```

---

## 🔄 Paso 7: Probar concurrencia

```bash
# En una terminal:
./servidor_mq &

# Ejecutar 3 clientes simultáneamente
./app-cliente-mq &
./app-cliente-mq &
./app-cliente-mq &

# Esperar a que terminen
wait

# Ver el log del servidor
fg  # O killall servidor_mq
```

---

## 🧹 Paso 8: Limpiar el entorno

```bash
# Matar el servidor si está corriendo
killall servidor_mq

# Limpiar colas
./limpiar_colas.sh

# O manualmente:
rm -f /dev/mqueue/cola_*

# Verificar que no quedan colas
ls -la /dev/mqueue/

# Limpiar archivos compilados
make clean
```

---

## 🔍 Verificaciones importantes

### 1. Verificar bibliotecas dinámicas

```bash
ldd app-cliente
# Debe mostrar: libclaves.so => ./libclaves.so

ldd app-cliente-mq
# Debe mostrar: libproxyclaves.so => ./libproxyclaves.so

ldd servidor_mq
# Debe mostrar: libclaves.so => ./libclaves.so
```

### 2. Verificar colas de mensajes

```bash
# Ver colas activas
ls -lh /dev/mqueue/

# Deberías ver:
# - cola_servidor_sd (cuando el servidor está corriendo)
# - cola_cliente_sd_XXXXX (cuando un cliente está esperando respuesta)
```

### 3. Verificar procesos

```bash
# Ver si el servidor está corriendo
ps aux | grep servidor_mq

# Ver clientes activos
ps aux | grep app-cliente
```

---

## ⚠️ Solución de problemas en Guernika

### Problema: "Permission denied" al compilar
```bash
chmod +x *.sh
chmod 644 *.c *.h
```

### Problema: "mqueue.h: No such file"
Guernika debe tener POSIX instalado. Si no, contactar al administrador.

### Problema: "Cannot open shared object file"
```bash
# Verificar que las bibliotecas están en el mismo directorio
ls -lh *.so

# Si es necesario, configurar LD_LIBRARY_PATH:
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```

### Problema: Colas bloqueadas
```bash
# Eliminar todas las colas manualmente
rm -rf /dev/mqueue/cola_*

# Reintentar
```

### Problema: El servidor no arranca
```bash
# Ver errores detallados
./servidor_mq 2>&1 | tee servidor_debug.log

# Verificar que no hay otra instancia corriendo
killall -9 servidor_mq
```

---

## 📊 Verificar resultados esperados

### Parte A (local)
- ✅ Total de pruebas: 26 (aproximadamente)
- ✅ Pruebas exitosas: 26
- ✅ Pruebas fallidas: 0
- ✅ Mensaje final: "¡Todas las pruebas pasaron correctamente!"

### Parte B (distribuida)
- ✅ Mismo resultado que Parte A
- ✅ Servidor muestra logs de procesamiento
- ✅ No hay errores de comunicación (código -2)

---

## 📸 Capturar evidencia (opcional)

```bash
# Guardar output de las pruebas
./app-cliente > pruebas_local.txt 2>&1
./app-cliente-mq > pruebas_distribuida.txt 2>&1

# Copiar a tu máquina local
scp usuario@guernika.i2basque.es:~/ejercicio1/pruebas_*.txt .
```

---

## 📝 Checklist final en Guernika

- [ ] Compilación sin errores ni warnings
- [ ] `make` genera todos los archivos (.so y ejecutables)
- [ ] `./app-cliente` pasa todas las pruebas
- [ ] `./servidor_mq` se inicia correctamente
- [ ] `./app-cliente-mq` pasa todas las pruebas
- [ ] Prueba de concurrencia funciona (3+ clientes simultáneos)
- [ ] Las bibliotecas se enlazan correctamente (ldd)
- [ ] No quedan procesos huérfanos ni colas bloqueadas

---

## 🏁 Desconectar de Guernika

```bash
# Limpiar antes de salir
make clean
rm -f /dev/mqueue/cola_*

# Salir
exit
```

---

**¡Listo!** Si todas las pruebas pasan en Guernika, el ejercicio está completo. Solo falta crear la memoria en PDF.

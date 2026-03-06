#!/bin/bash
# test_distribuido.sh - Script de pruebas para la versión distribuida
# Ejercicio Evaluable 1 - Sistemas Distribuidos

echo "=========================================="
echo "  PRUEBAS VERSIÓN DISTRIBUIDA"
echo "  Ejercicio Evaluable 1"
echo "=========================================="
echo ""

# Verificar que los ejecutables existen
if [ ! -f "./servidor_mq" ]; then
    echo "ERROR: servidor_mq no encontrado. Ejecute 'make' primero."
    exit 1
fi

if [ ! -f "./app-cliente-mq" ]; then
    echo "ERROR: app-cliente-mq no encontrado. Ejecute 'make' primero."
    exit 1
fi

# Limpiar colas previas
echo "[1/5] Limpiando colas anteriores..."
rm -f /dev/mqueue/cola_* 2>/dev/null
sleep 1

# Iniciar servidor en background
echo "[2/5] Iniciando servidor..."
./servidor_mq > servidor.log 2>&1 &
SERVIDOR_PID=$!
echo "  Servidor iniciado (PID: $SERVIDOR_PID)"
sleep 2

# Verificar que el servidor está corriendo
if ! ps -p $SERVIDOR_PID > /dev/null; then
    echo "ERROR: El servidor no pudo iniciarse"
    cat servidor.log
    exit 1
fi

# Ejecutar cliente de pruebas
echo "[3/5] Ejecutando cliente de pruebas..."
echo ""
./app-cliente-mq
CLIENTE_RESULT=$?
echo ""

# Ejecutar prueba de concurrencia (3 clientes simultáneos)
echo "[4/5] Prueba de concurrencia (3 clientes simultáneos)..."
./app-cliente-mq > cliente1.log 2>&1 &
PID1=$!
./app-cliente-mq > cliente2.log 2>&1 &
PID2=$!
./app-cliente-mq > cliente3.log 2>&1 &
PID3=$!

# Esperar a que terminen
wait $PID1
wait $PID2
wait $PID3

echo "  ✓ Clientes concurrentes finalizados"
echo ""

# Terminar servidor
echo "[5/5] Terminando servidor..."
kill $SERVIDOR_PID 2>/dev/null
wait $SERVIDOR_PID 2>/dev/null
echo "  ✓ Servidor terminado"
echo ""

# Limpiar colas
echo "Limpiando colas..."
rm -f /dev/mqueue/cola_* 2>/dev/null

# Mostrar resultados
echo "=========================================="
echo "  RESULTADOS"
echo "=========================================="
echo "Log del servidor: servidor.log"
echo "Logs de clientes concurrentes: cliente1.log, cliente2.log, cliente3.log"
echo ""

if [ $CLIENTE_RESULT -eq 0 ]; then
    echo "✓ Todas las pruebas pasaron correctamente"
    exit 0
else
    echo "✗ Algunas pruebas fallaron (código: $CLIENTE_RESULT)"
    exit 1
fi

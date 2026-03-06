#!/bin/bash
# test_local.sh - Script de pruebas para la versión no distribuida
# Ejercicio Evaluable 1 - Sistemas Distribuidos

echo "=========================================="
echo "  PRUEBAS VERSIÓN NO DISTRIBUIDA"
echo "  Ejercicio Evaluable 1"
echo "=========================================="
echo ""

# Verificar que el ejecutable existe
if [ ! -f "./app-cliente" ]; then
    echo "ERROR: app-cliente no encontrado. Ejecute 'make local' primero."
    exit 1
fi

# Ejecutar cliente de pruebas
echo "Ejecutando plan de pruebas..."
echo ""
./app-cliente
RESULT=$?
echo ""

# Mostrar resultado
echo "=========================================="
echo "  RESULTADO"
echo "=========================================="

if [ $RESULT -eq 0 ]; then
    echo "✓ Todas las pruebas pasaron correctamente"
    exit 0
else
    echo "✗ Algunas pruebas fallaron (código: $RESULT)"
    exit 1
fi

#!/bin/bash
# limpiar_colas.sh - Script para limpiar colas de mensajes POSIX
# Ejercicio Evaluable 1 - Sistemas Distribuidos

echo "======================================"
echo "  Limpieza de Colas de Mensajes POSIX"
echo "======================================"
echo ""

# Verificar si /dev/mqueue existe
if [ ! -d "/dev/mqueue" ]; then
    echo "ERROR: /dev/mqueue no existe en este sistema"
    exit 1
fi

# Mostrar colas existentes
echo "Colas actuales en /dev/mqueue:"
ls -lh /dev/mqueue/ 2>/dev/null || echo "  (vacío)"
echo ""

# Contar colas relacionadas con el ejercicio
NUM_COLAS=$(ls /dev/mqueue/cola_* 2>/dev/null | wc -l)

if [ $NUM_COLAS -eq 0 ]; then
    echo "No hay colas del ejercicio para limpiar."
    exit 0
fi

echo "Se encontraron $NUM_COLAS cola(s) del ejercicio."
echo ""

# Preguntar confirmación
read -p "¿Desea eliminar todas las colas? (s/n): " confirmacion

if [ "$confirmacion" = "s" ] || [ "$confirmacion" = "S" ]; then
    echo ""
    echo "Eliminando colas..."
    
    # Eliminar cola del servidor
    if [ -e "/dev/mqueue/cola_servidor_sd" ]; then
        rm /dev/mqueue/cola_servidor_sd
        echo "  ✓ Cola del servidor eliminada"
    fi
    
    # Eliminar colas de clientes
    contador=0
    for cola in /dev/mqueue/cola_cliente_sd_*; do
        if [ -e "$cola" ]; then
            rm "$cola"
            contador=$((contador + 1))
        fi
    done
    
    if [ $contador -gt 0 ]; then
        echo "  ✓ $contador cola(s) de cliente eliminadas"
    fi
    
    echo ""
    echo "Limpieza completada."
else
    echo ""
    echo "Operación cancelada."
fi

echo ""
echo "Colas restantes:"
ls -lh /dev/mqueue/ 2>/dev/null || echo "  (vacío)"

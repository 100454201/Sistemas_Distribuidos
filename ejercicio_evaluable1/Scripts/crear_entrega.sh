#!/bin/bash
# crear_entrega.sh - Script para crear el archivo de entrega
# Ejercicio Evaluable 1 - Sistemas Distribuidos

echo "=========================================="
echo "  CREAR ARCHIVO DE ENTREGA"
echo "  ejercicio_evaluable1.zip"
echo "=========================================="
echo ""

# Nombre del archivo de salida
ARCHIVO_ZIP="ejercicio_evaluable1.zip"

# Archivos a incluir en la entrega
ARCHIVOS=(
    # Código fuente
    "claves.h"
    "claves.c"
    "app-cliente.c"
    "servidor-mq.c"
    "proxy-mq.c"
    "protocolo.h"
    
    # Makefile
    "Makefile"
    
    # Memoria (debe crearse manualmente)
    # "memoria.pdf"
    
    # Scripts de ayuda
    "limpiar_colas.sh"
    "test_local.sh"
    "test_distribuido.sh"
    
    # Documentación
    "README.md"
)

# Verificar que todos los archivos existen
echo "[1/3] Verificando archivos..."
FALTA_ARCHIVO=0

for archivo in "${ARCHIVOS[@]}"; do
    if [ ! -f "$archivo" ]; then
        echo "  ✗ Falta: $archivo"
        FALTA_ARCHIVO=1
    else
        echo "  ✓ $archivo"
    fi
done

# Verificar si existe la memoria
if [ ! -f "memoria.pdf" ]; then
    echo ""
    echo "  ⚠️  ADVERTENCIA: memoria.pdf no encontrado"
    echo "     Recuerda crear la memoria (máx. 5 páginas) antes de entregar"
    echo ""
    read -p "¿Desea continuar sin la memoria? (s/n): " continuar
    if [ "$continuar" != "s" ] && [ "$continuar" != "S" ]; then
        echo "Operación cancelada."
        exit 1
    fi
else
    echo "  ✓ memoria.pdf"
    ARCHIVOS+=("memoria.pdf")
fi

if [ $FALTA_ARCHIVO -eq 1 ]; then
    echo ""
    echo "ERROR: Faltan archivos necesarios"
    exit 1
fi

# Eliminar zip anterior si existe
if [ -f "$ARCHIVO_ZIP" ]; then
    echo ""
    echo "[2/3] Eliminando $ARCHIVO_ZIP anterior..."
    rm "$ARCHIVO_ZIP"
fi

# Crear archivo zip
echo ""
echo "[3/3] Creando $ARCHIVO_ZIP..."
zip -q "$ARCHIVO_ZIP" "${ARCHIVOS[@]}"

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "  ✓ ARCHIVO CREADO EXITOSAMENTE"
    echo "=========================================="
    echo ""
    echo "Archivo: $ARCHIVO_ZIP"
    echo "Tamaño:  $(du -h "$ARCHIVO_ZIP" | cut -f1)"
    echo ""
    echo "Contenido:"
    unzip -l "$ARCHIVO_ZIP"
    echo ""
    echo "⚠️  RECUERDA:"
    echo "  1. Verificar que memoria.pdf está incluido"
    echo "  2. Revisar que todos los archivos estén presentes"
    echo "  3. Fecha límite: 15/03/2026 - 23:55"
    echo "  4. Entregar por Aula Global"
    echo ""
else
    echo ""
    echo "ERROR al crear el archivo zip"
    exit 1
fi

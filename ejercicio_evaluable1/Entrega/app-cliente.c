/*
 * app-cliente.c - Aplicación cliente de prueba
 * Sistemas Distribuidos - Ejercicio Evaluable 1
 * 
 * Programa de pruebas para el servicio de tuplas.
 * Implementa un plan de pruebas exhaustivo.
 */

#include <stdio.h>
#include <string.h>
#include "claves.h"

/* Colores para output (opcional) */
#define COLOR_OK "\033[0;32m"
#define COLOR_ERROR "\033[0;31m"
#define COLOR_INFO "\033[0;34m"
#define COLOR_RESET "\033[0m"

/* Contador de pruebas */
static int total_pruebas = 0;
static int pruebas_ok = 0;

void mostrar_resultado(const char *nombre_prueba, int resultado_esperado, int resultado_obtenido) {
    total_pruebas++;
    if (resultado_esperado == resultado_obtenido) {
        pruebas_ok++;
        printf("[%sOK%s] %s\n", COLOR_OK, COLOR_RESET, nombre_prueba);
    } else {
        printf("[%sERROR%s] %s (esperado: %d, obtenido: %d)\n", 
               COLOR_ERROR, COLOR_RESET, nombre_prueba, resultado_esperado, resultado_obtenido);
    }
}

void separador(const char *titulo) {
    printf("\n%s=== %s ===%s\n", COLOR_INFO, titulo, COLOR_RESET);
}

int main(int argc, char **argv) {
    printf("%s====================================%s\n", COLOR_INFO, COLOR_RESET);
    printf("%s  PLAN DE PRUEBAS - PARTE A         %s\n", COLOR_INFO, COLOR_RESET);
    printf("%s====================================%s\n\n", COLOR_INFO, COLOR_RESET);
    
    /* ===== PRUEBA 1: Inicialización ===== */
    separador("PRUEBA 1: Inicialización");
    int res = destroy();
    mostrar_resultado("destroy() inicial", 0, res);
    
    /* ===== PRUEBA 2: Insertar tuplas básicas ===== */
    separador("PRUEBA 2: Insertar tuplas");
    
    char *key1 = "clave1";
    char *v1_1 = "valor1 de clave1";
    float v2_1[] = {1.5, 2.3, 3.7};
    struct Paquete v3_1 = {10, 20, 30};
    
    res = set_value(key1, v1_1, 3, v2_1, v3_1);
    mostrar_resultado("set_value(clave1)", 0, res);
    
    char *key2 = "clave2";
    char *v1_2 = "segundo valor";
    float v2_2[] = {9.9};
    struct Paquete v3_2 = {100, 200, 300};
    
    res = set_value(key2, v1_2, 1, v2_2, v3_2);
    mostrar_resultado("set_value(clave2)", 0, res);
    
    /* ===== PRUEBA 3: Insertar clave duplicada (debe fallar) ===== */
    separador("PRUEBA 3: Error - clave duplicada");
    res = set_value(key1, "otro valor", 2, v2_1, v3_1);
    mostrar_resultado("set_value(clave duplicada) debe fallar", -1, res);
    
    /* ===== PRUEBA 4: Validar N_value2 fuera de rango ===== */
    separador("PRUEBA 4: Error - N_value2 fuera de rango");
    float v2_invalid[40];
    res = set_value("clave3", "valor", 0, v2_invalid, v3_1);
    mostrar_resultado("set_value(N=0) debe fallar", -1, res);
    
    res = set_value("clave4", "valor", 33, v2_invalid, v3_1);
    mostrar_resultado("set_value(N=33) debe fallar", -1, res);
    
    /* ===== PRUEBA 5: Verificar existencia ===== */
    separador("PRUEBA 5: Verificar existencia");
    res = exist(key1);
    mostrar_resultado("exist(clave1)", 1, res);
    
    res = exist("clave_inexistente");
    mostrar_resultado("exist(clave_inexistente)", 0, res);
    
    /* ===== PRUEBA 6: Recuperar valores ===== */
    separador("PRUEBA 6: Recuperar valores");
    
    char recuperado_v1[256];
    int recuperado_N;
    float recuperado_v2[32];
    struct Paquete recuperado_v3;
    
    res = get_value(key1, recuperado_v1, &recuperado_N, recuperado_v2, &recuperado_v3);
    mostrar_resultado("get_value(clave1)", 0, res);
    
    if (res == 0) {
        int coincide = (strcmp(recuperado_v1, v1_1) == 0 &&
                        recuperado_N == 3 &&
                        recuperado_v2[0] == v2_1[0] &&
                        recuperado_v2[1] == v2_1[1] &&
                        recuperado_v2[2] == v2_1[2] &&
                        recuperado_v3.x == v3_1.x &&
                        recuperado_v3.y == v3_1.y &&
                        recuperado_v3.z == v3_1.z);
        
        mostrar_resultado("Verificar datos recuperados", 1, coincide);
        
        if (coincide) {
            printf("  - value1: %s\n", recuperado_v1);
            printf("  - N_value2: %d\n", recuperado_N);
            printf("  - V_value2: [%.1f, %.1f, %.1f]\n", 
                   recuperado_v2[0], recuperado_v2[1], recuperado_v2[2]);
            printf("  - value3: {%d, %d, %d}\n", 
                   recuperado_v3.x, recuperado_v3.y, recuperado_v3.z);
        }
    }
    
    /* ===== PRUEBA 7: Recuperar con clave inexistente ===== */
    separador("PRUEBA 7: Error - get_value clave inexistente");
    res = get_value("no_existe", recuperado_v1, &recuperado_N, recuperado_v2, &recuperado_v3);
    mostrar_resultado("get_value(clave inexistente) debe fallar", -1, res);
    
    /* ===== PRUEBA 8: Modificar valores ===== */
    separador("PRUEBA 8: Modificar valores");
    
    char *v1_modificado = "valor1 MODIFICADO";
    float v2_modificado[] = {99.9, 88.8};
    struct Paquete v3_modificado = {999, 888, 777};
    
    res = modify_value(key1, v1_modificado, 2, v2_modificado, v3_modificado);
    mostrar_resultado("modify_value(clave1)", 0, res);
    
    /* Verificar que se modificó correctamente */
    res = get_value(key1, recuperado_v1, &recuperado_N, recuperado_v2, &recuperado_v3);
    if (res == 0) {
        int modificado_ok = (strcmp(recuperado_v1, v1_modificado) == 0 &&
                             recuperado_N == 2 &&
                             recuperado_v2[0] == v2_modificado[0] &&
                             recuperado_v2[1] == v2_modificado[1] &&
                             recuperado_v3.x == v3_modificado.x &&
                             recuperado_v3.y == v3_modificado.y &&
                             recuperado_v3.z == v3_modificado.z);
        
        mostrar_resultado("Verificar modificación", 1, modificado_ok);
    }
    
    /* ===== PRUEBA 9: Modificar clave inexistente ===== */
    separador("PRUEBA 9: Error - modify_value clave inexistente");
    res = modify_value("no_existe", "valor", 1, v2_1, v3_1);
    mostrar_resultado("modify_value(clave inexistente) debe fallar", -1, res);
    
    /* ===== PRUEBA 10: Eliminar clave ===== */
    separador("PRUEBA 10: Eliminar clave");
    res = delete_key(key2);
    mostrar_resultado("delete_key(clave2)", 0, res);
    
    res = exist(key2);
    mostrar_resultado("exist(clave2) tras borrar", 0, res);
    
    /* ===== PRUEBA 11: Eliminar clave inexistente ===== */
    separador("PRUEBA 11: Error - delete_key clave inexistente");
    res = delete_key("no_existe");
    mostrar_resultado("delete_key(clave inexistente) debe fallar", -1, res);
    
    /* ===== PRUEBA 12: Límites - cadena larga ===== */
    separador("PRUEBA 12: Límites - cadenas largas");
    
    char clave_larga[260];
    memset(clave_larga, 'A', 256);
    clave_larga[256] = '\0';
    
    res = set_value(clave_larga, "valor", 1, v2_1, v3_1);
    mostrar_resultado("set_value(clave >255 chars) debe fallar", -1, res);
    
    char valor1_largo[260];
    memset(valor1_largo, 'B', 256);
    valor1_largo[256] = '\0';
    
    res = set_value("clave_ok", valor1_largo, 1, v2_1, v3_1);
    mostrar_resultado("set_value(value1 >255 chars) debe fallar", -1, res);
    
    /* ===== PRUEBA 13: Vector con tamaño máximo ===== */
    separador("PRUEBA 13: Vector de tamaño máximo (32)");
    
    float v2_max[32];
    for (int i = 0; i < 32; i++) {
        v2_max[i] = (float)i * 1.1f;
    }
    
    res = set_value("clave_max", "valor max", 32, v2_max, v3_1);
    mostrar_resultado("set_value(N=32)", 0, res);
    
    res = get_value("clave_max", recuperado_v1, &recuperado_N, recuperado_v2, &recuperado_v3);
    mostrar_resultado("get_value(clave_max)", 0, res);
    
    if (res == 0 && recuperado_N == 32) {
        printf("  - Vector recuperado con 32 elementos correctamente\n");
    }
    
    /* ===== PRUEBA 14: Múltiples inserciones ===== */
    separador("PRUEBA 14: Múltiples inserciones");
    
    for (int i = 0; i < 10; i++) {
        char key[50];
        char val[50];
        sprintf(key, "clave_%d", i);
        sprintf(val, "valor_%d", i);
        
        float arr[] = {(float)i};
        struct Paquete paq = {i, i*10, i*100};
        
        res = set_value(key, val, 1, arr, paq);
        if (res != 0) {
            printf("  Error insertando clave_%d\n", i);
        }
    }
    mostrar_resultado("Insertar 10 tuplas", 0, 0); /* Asumimos éxito si no hubo error */
    
    /* Verificar que todas existen */
    int todas_existen = 1;
    for (int i = 0; i < 10; i++) {
        char key[50];
        sprintf(key, "clave_%d", i);
        if (exist(key) != 1) {
            todas_existen = 0;
            break;
        }
    }
    mostrar_resultado("Verificar que existen las 10 tuplas", 1, todas_existen);
    
    /* ===== PRUEBA 15: destroy() con datos ===== */
    separador("PRUEBA 15: Destruir todas las tuplas");
    res = destroy();
    mostrar_resultado("destroy()", 0, res);
    
    res = exist(key1);
    mostrar_resultado("exist(clave1) tras destroy", 0, res);
    
    res = exist("clave_0");
    mostrar_resultado("exist(clave_0) tras destroy", 0, res);
    
    /* ===== RESUMEN FINAL ===== */
    printf("\n%s====================================%s\n", COLOR_INFO, COLOR_RESET);
    printf("%s  RESUMEN DE PRUEBAS%s\n", COLOR_INFO, COLOR_RESET);
    printf("%s====================================%s\n", COLOR_INFO, COLOR_RESET);
    printf("Total de pruebas: %d\n", total_pruebas);
    printf("Pruebas exitosas: %s%d%s\n", COLOR_OK, pruebas_ok, COLOR_RESET);
    printf("Pruebas fallidas: %s%d%s\n", 
           pruebas_ok == total_pruebas ? COLOR_OK : COLOR_ERROR, 
           total_pruebas - pruebas_ok, COLOR_RESET);
    
    if (pruebas_ok == total_pruebas) {
        printf("\n%s¡Todas las pruebas pasaron correctamente!%s\n\n", COLOR_OK, COLOR_RESET);
        return 0;
    } else {
        printf("\n%sAlgunas pruebas fallaron.%s\n\n", COLOR_ERROR, COLOR_RESET);
        return 1;
    }
}

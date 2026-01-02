#!/bin/bash

# Uso: ./switch.sh [linux|esp32]

TARGET=$1

if [ "$TARGET" == "linux" ]; then
    echo ">> Cambiando entorno a: LINUX"
    
    # 1. Creamos enlace simbólico (ln -sf fuerza la sobreescritura si existe)
    ln -sf build_linux/compile_commands.json compile_commands.json
    
    # 2. Guardamos el estado para el script de build
    echo "linux" > .current_target
    
    echo "✅ compile_commands.json apunta ahora a build_linux/"

elif [ "$TARGET" == "esp32" ]; then
    echo ">> Cambiando entorno a: ESP32"
    
    # 1. Creamos enlace simbólico
    ln -sf build/compile_commands.json compile_commands.json
    
    # 2. Guardamos el estado
    echo "esp32" > .current_target
    
    echo "✅ compile_commands.json apunta ahora a build/"

else
    echo "❌ Error: Debes especificar el target."
    echo "Uso: ./switch.sh linux"
    echo "     ./switch.sh esp32"
    exit 1
fi

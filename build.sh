#!/bin/bash

# Comprobamos si existe el archivo de estado
if [ ! -f .current_target ]; then
    echo "❌ No se ha seleccionado ningún target."
    echo "Ejecuta primero: ./switch.sh [linux|esp32]"
    exit 1
fi

TARGET=$(cat .current_target)

# ==========================================
# BLOQUE LINUX
# ==========================================
if [ "$TARGET" == "linux" ]; then
    echo "🐧 Entorno seleccionado: LINUX"
    
    mkdir -p build_linux
    cd build_linux
    
    # Configuración de CMake (tus flags)
    cmake -DBUILD_FOR_LINUX=ON \
		  -DCMAKE_BUILD_TYPE=Debug \
		  -DCMAKE_CXX_FLAGS="-fsanitize=address -g" \
          -DCMAKE_C_FLAGS="-fsanitize=address -g" \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DCMAKE_C_COMPILER=/usr/bin/gcc \
          -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
          ..
    
    # Compilar
    cmake --build .
    echo "✅ Build Linux finalizado."

# ==========================================
# BLOQUE ESP32
# ==========================================
elif [ "$TARGET" == "esp32" ]; then
    echo "🦎 Entorno seleccionado: ESP32"
    
    # --- AUTO-ACTIVACIÓN DEL ENTORNO ESP-IDF ---
    # Comprobamos si 'idf.py' es un comando reconocido.
    # Si no lo es, cargamos el export.sh
    if ! command -v idf.py &> /dev/null; then
        export IDF_TOOLS_PATH="$HOME/.local/share/espressif"
        source $HOME/dev/esp-idf/export.sh
    fi
	idf.py reconfigure
    idf.py build
	idf.py flash
	idf.py monitor
   
    echo "✅ Build ESP32 finalizado."

else
    echo "❌ Target desconocido en .current_target: $TARGET"
fi

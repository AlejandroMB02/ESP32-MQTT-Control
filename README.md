# ESP32 MQTT Control

[![Build with ESP-IDF](https://img.shields.io/badge/build-ESP--IDF-blue?logo=espressif)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)  
[![Docker](https://img.shields.io/badge/Docker-ready-blue?logo=docker)](https://www.docker.com/)  
[![MQTT](https://img.shields.io/badge/MQTT-protocol-purple?logo=eclipsemosquitto)](https://mqtt.org/)  
[![ESP32](https://img.shields.io/badge/board-ESP32-orange?logo=espressif)](https://www.espressif.com/en/products/socs/esp32)  

Este proyecto implementa una arquitectura IoT sencilla basada en **ESP32**, un sensor **DHT11** de temperatura y humedad, y una **interfaz web** que permite visualizar los datos y controlar una luz de manera remota mediante **MQTT**.  
La comunicación entre el ESP32 y la web se realiza a través de un **broker MQTT**, y tanto la web como el broker se despliegan con **Docker** para facilitar su configuración y portabilidad.

---

## 🚀 Arquitectura del Proyecto

- **ESP32**: Obtiene los datos de temperatura y humedad mediante el sensor **DHT11** y se comunica vía **MQTT**.  
- **MQTT Broker (Eclipse Mosquitto)**: Actúa como intermediario entre el ESP32 y la aplicación web.  
- **Dashboard Web**: Desplegado en Docker, permite:
  - Visualizar los datos de temperatura y humedad.
  - Encender y apagar una luz enviando comandos al ESP32.

👉 Aquí puedes incluir un **diagrama de la arquitectura** (ESP32 ↔ MQTT Broker ↔ Web).  

---

## 🛠️ Requisitos

- **Hardware**:
  - ESP32
  - Sensor DHT11
  - LED o relay para controlar la luz

- **Software**:
  - [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
  - [Docker](https://docs.docker.com/get-docker/)
  - [Docker Compose](https://docs.docker.com/compose/)

---

## ⚙️ Instalación y Uso

1. Clonar el repositorio
   ```bash
   git clone [https://github.com/usuario/ESP32-MQTT-Control.git](https://github.com/AlejandroMB02/ESP32-MQTT-Control)
   cd ESP32-MQTT-Control/ESP32
2. Credenciales
  - Ajustar las credenciales de WiFi.
  - Configurar la IP/host del broker MQTT.
3. Compilar y flashear el ESP32
  ```bash
  idf.py build
  idf.py -p /dev/ttyUSB0 flash monitor

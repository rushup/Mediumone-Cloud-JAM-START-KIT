## All projects were built using Stm32CubeMX, current supported IDEs are IAR Workbench (EWARM) and Eclipse (SW4STM32)


## Adding new IDE

- Set this global symbol in IDE
    USE_STM32L4XX_NUCLEO or USE_STM32F4XX_NUCLEO
    
- Include the following paths

    - "../../../../../Common/App/Inc"
    - "../../../../../Common/App"
    - "../../../../../Common/MQTT-Paho/MQTTClient-C/src"
    - "../../../../../Common/MQTT-Paho/MQTTPacket/src"
    - "../../../../../Common/Nucleo/Components"
    - "../../../../../Common/Nucleo/X_NUCLEO_IKS01A2"
    - "../../../../../Common/Nucleo/X-NUCLEO-NFC01A1"
    - "../../../../../Common"

- Include all the source in /Common

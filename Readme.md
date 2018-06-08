All projects were built using Stm32CubeMX, current supported IDEs are IAR Workbench (EWARM),Eclipse (SW4STM32) and TrueSTUDIO.

## Open the project with existing IDEs

- System Workbench:
    Cube-Cloud-JAM/SW4STM32/SW4STM32/Cloud-JAM-SW4STM32
- TrueStudio:
    Cube-Cloud-JAM/SW4STM32/Cloud-JAM-TrueSTUDIO/TrueSTUDIO/Cloud-JAM-TrueSTUDIO
- IAR:
    Cube-Cloud-JAM/EWARM


The project also has working STM32CubeMX configuration for the different ides.

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

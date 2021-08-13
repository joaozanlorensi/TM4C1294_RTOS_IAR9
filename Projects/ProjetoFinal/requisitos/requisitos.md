**Título do projeto final:** Simulador de Veículo Autoguiado

**Requisitos Funcionais:**
- RF1: O Kit EK-TM4C1294XL deverá receber comandos do Computador
- RF2: O Kit EK-TM4C1294XL deverá reagir aos comandos do Computador
  - RF2.1: Uma instrução de aceleração enviada através do terminal deverá resultar no envio de uma mensagem de confirmação de aceleração enviada pelo veículo
  - RF2.2: Uma instrução de parada enviada através do terminal deverá resultar no envio de uma mensagem de confirmação de parada enviada pelo veículo
  - RF2.3: Uma instrução de realização de curva enviada através do terminal deverá resultar no envio de uma mensagem de confirmação de realização de curva enviada pelo veículo
  - RF2.4: Uma instrução de leitura de sensores enviada através do terminal deverá resultar no envio de uma mensagem que retorne um valor numerico ficticio que representaria a leitura de um sensor enviada pelo veículo
  - RF2.5: Uma instrução de reset enviada através do terminal deverá resultar no envio de uma mensagem de reset ao computador, que deverá retornar a simulação ao início
  - RF2.6: Uma instrução de parada de emergência enviada através do terminal deverá resultar no envio de uma mensagem de confirmação de parada de emergência enviada pelo veículo

**Requisitos Não Funcionais:**

- RNF1: O código deverá utilizar funções do CMSIS-RTOS
- RNF2: O sistema deverá operar a partir de uma máquina de estados
- RNF3: A interface de comunicação com o computador deverá ser a interface serial UART
- RNF4: O sistema deverá utilizar de programação concorrente e de interrupções
- RNF5: O sistema deverá piscar um LED do Kit ao receber uma instrução de reset
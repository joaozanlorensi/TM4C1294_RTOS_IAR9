**Título do projeto final:** Simulador de Veículo Autoguiado

**Requisitos Funcionais:**

- RF1: O veículo deverá realizar a leitura periódica dos dados coletados pelos sensores físicos que o integram
- RF2: O veículo deverá percorrer uma pista de corrida de forma autônoma
- RF3: O veículo não deverá colidir com os obstáculos da pista ou com as margens da pista que percorrer
- RF4: O veículo deverá acelerar para frente automaticamente ao início da corrida
- RF5: O veículo deverá aumentar a sua aceleração se não detectar obstáculos a uma distância limite à sua frente, que deverá ser calculada dinamicamente de forma a garantir que ele não colida
- RF6: O veículo deverá desviar automaticamente se houver obstáculos à uma distância limite da sua frente, que deverá ser calculada dinamicamente conforme a velocidade em que o veículo estiver
  - RF6.1: Para desviar, o veículo deverá freiar, virar em um ângulo adequado para que não haja a sua colisão e acelerar na direção a que ele virou
  - RF6.2: O veículo deverá ser capaz de determinar o ângulo adequado para virar de forma que não haja colisão em nenhum ponto da sua superfície com os obstáculos 
  - RF6.3: Se um obstáculo estiver a 5 metros da sua frente, o veículo deverá realizar uma parada emergencial, seguida de uma virada em um ângulo que o permita não colidir, e de uma aceleração
- RF7: Ao final de uma volta pela pista, o veículo deverá freiar

**Requisitos Não Funcionais:**

- RNF1: O período de leitura dos dados coletados pelos sensores do veículo deverá ser de 200 milisegundos
- RNF2: O veículo deverá piscar um LED quando o simulador receber uma instrução de reset
- RNF3: O veículo deverá ser capaz de coletar estatísticas da sua operação e de transmiti-las a um computador ao final de cada volta na pista
  - RNF3.1: O veículo deverá transmitir qual foi a sua máxima velocidade atingida
  - RNF3.2: O veículo deverá transmitir qual foi a quantidade de obstáculos de que ele desviou
  - RNF3.3: O veículo deverá transmitir qual foi o tempo total que ele levou para completar a volta
  - RNF3.4: O veículo deverá transmitir qual foi a sua velocidade média ao longo do trajeto 
  - RNF3.5: As estatísticas de operação deverão ser exibidas ao usuário através da interface visual do simulador
- RNF4: A interação entre o usuário externo e o veículo deverá ser feita de forma remota

**Restrições**

- R1: O sistema eletrônico do veículo deverá utilizar funções do CMSIS-RTOS
- R2: O protocolo de comunicação entre o veículo e o usuário externo capaz de controlar a corrida e visualizar suas estatísticas deverá ser o protocolo Bluetooth
- R3: O veículo deverá operar a partir de uma máquina de estados com 6 estados: Iniciando; Movendo para frente; Virando para a direita; Virando para a esquerda; Freiando; Realizando parada emergencial.
- R4: O veículo deverá utilizar o Kit EK-TM4C1294XL
- R5: O veículo deverá evitar cálculos redundantes


**Regras de Operação:**

- RO1: A corrida deverá ser iniciada mediante à inicialização do Software de simulação de corrida, à seleção dos parâmetros para a corrida, e ao envio de uma instrução de início
- RO2: O usuário externo poderá enviar uma instrução de Reset a qualquer momento para o simulador
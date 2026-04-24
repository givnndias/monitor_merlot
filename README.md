# Vinheria Inteligente com FIWARE, ESP32 e Dashboard Web

## Integrantes da equipe

- **Giovanna Oliveira Ferreira Dias** — RM: 566647
- **Maria Laura Druzeic** — RM: 566634
- **Marianne Mukai Nishikawa** — RM: 568001

---

## Visão geral

A **Vinheria Inteligente** é um projeto de Internet das Coisas desenvolvido para monitorar as condições ambientais ideais de armazenamento de vinhos, com foco em **temperatura**, **umidade** e **luminosidade**.

O sistema utiliza um **ESP32** conectado a sensores e atuadores, integrado à plataforma **FIWARE** por meio de **MQTT**. Os dados enviados são processados pelo **IoT Agent**, armazenados no **Orion Context Broker** como estado atual e persistidos historicamente no **STH-Comet**. Depois, esses dados são exibidos em um **dashboard web em Python**, disponível na porta **5000**.

O projeto também conta com:
- leitura local no **display LCD I2C**
- **alerta visual** com LED onboard
- **alerta sonoro** com buzzer
- consulta e testes via **Postman**
- simulação no **Wokwi**

O código do ESP32 mostra explicitamente a leitura de luminosidade, temperatura e umidade, além do envio MQTT dos atributos `l`, `t` e `h`, que são mapeados no FIWARE para `luminosity`, `temperature` e `humidity`. 

---

## Objetivo do projeto

O objetivo do sistema é:

- monitorar continuamente o ambiente de uma vinheria
- identificar condições fora da faixa ideal
- enviar os dados para o FIWARE em tempo real
- armazenar histórico das medições
- disponibilizar um dashboard web para visualização dos dados
- acionar alertas locais quando houver anomalias

---

## Arquitetura da solução

O fluxo do sistema funciona assim:

1. O **ESP32** lê os sensores de luminosidade, temperatura e umidade.
2. Os dados são enviados via **MQTT** para o **IoT Agent MQTT**.
3. O **IoT Agent** atualiza a entidade no **Orion Context Broker**.
4. O **Orion** mantém o estado atual da entidade.
5. O **STH-Comet** armazena o histórico das medições no banco.
6. O **dashboard web em Python** consome a API do STH-Comet na porta **8666**.
7. O usuário visualiza os gráficos históricos na porta **5000**.

---

## Hardware utilizado

| Componente | Função |
|---|---|
| **ESP32** | Microcontrolador principal |
| **DHT22** | Leitura de temperatura e umidade |
| **LDR** | Leitura de luminosidade |
| **Buzzer** | Alerta sonoro |
| **LED onboard (GPIO 2)** | Alerta visual |
| **LCD I2C 16x2** | Exibição local das medições |

O código do projeto define os pinos principais como:
- `DHT_PIN = 14`
- `LDR_PIN = 34`
- `BUZZER_PIN = 27`
- `LED_PIN = 2` :contentReference[oaicite:2]{index=2}

---

## Faixas ideais monitoradas

No código do ESP32, os limites de segurança usados para a vinheria são:

- **Temperatura mínima:** 12 °C
- **Temperatura máxima:** 18 °C
- **Umidade mínima:** 60%
- **Umidade máxima:** 70%
- **Luminosidade máxima:** 30% :contentReference[oaicite:3]{index=3}

Quando esses limites são ultrapassados:
- o LCD exibe alerta
- o buzzer é acionado
- o sistema envia comandos MQTT de ativação/desativação remota 

---

## Tecnologias utilizadas

| Camada | Tecnologia | Finalidade |
|---|---|---|
| Dispositivo IoT | **ESP32 + Arduino IDE** | Leitura de sensores e publicação MQTT |
| Simulação | **Wokwi** | Teste do circuito e do código |
| Plataforma IoT | **FIWARE** | Integração, contexto e histórico |
| Broker de contexto | **Orion Context Broker** | Estado atual da entidade |
| Agente IoT | **IoT Agent MQTT** | Recebimento dos dados MQTT |
| Histórico | **STH-Comet** | Persistência histórica |
| Banco de dados | **MongoDB** | Armazenamento do histórico |
| Dashboard | **Python + Flask** | Interface web |
| Visualização | **Matplotlib / Plotly** | Geração de gráficos |
| Testes | **Postman** | Configuração e validação da API |
| Nuvem | **VM Linux no Azure/Cloud** | Hospedagem do FIWARE e dashboard |

A base do README anterior enviado também descrevia a solução hospedada em VM Linux com FIWARE, dashboard Flask e acesso por portas públicas. 

---

## Configuração do ESP32 e MQTT

O código do ESP32 utiliza as seguintes configurações principais:

### Rede
- SSID: `Wokwi-GUEST`
- Senha: vazia no ambiente Wokwi

### Broker MQTT
- broker configurado no código
- porta MQTT: `1883`

### Identificação
- `device_id`: `lamp001`
- `entity_name`: `urn:ngsi-id:lamp:001`
- `entity_type`: `lamp` :contentReference[oaicite:6]{index=6}

### Tópicos MQTT usados
- `/TEF/lamp001/cmd` → recebe comandos remotos
- `/TEF/lamp001/attrs` → publica estado do LED
- `/TEF/lamp001/attrs/l` → publica luminosidade
- `/TEF/lamp001/attrs/t` → publica temperatura
- `/TEF/lamp001/attrs/h` → publica umidade
- `/TEF/lamp001/attrs/b` → publica estado do buzzer 

Esses tópicos precisam estar coerentes com o provisioning do FIWARE no Postman.

---

## Configuração no Wokwi

-Link Wokwi: https://wokwi.com/projects/461824683651877889

No Wokwi, o projeto deve conter:
- ESP32
- sensor DHT22
- LDR
- buzzer
- display LCD I2C

### Observações importantes
- O Wokwi permite simular a variação dos sensores.
- Para gerar histórico no STH-Comet, os valores precisam mudar ao longo do tempo.
- Se temperatura e umidade não variarem, o histórico desses atributos pode não aparecer como esperado.
- O LCD mostra as leituras em tempo real e também mensagens de alerta. Isso aparece na lógica de `checkAlerts` do código. 

### Sequência recomendada de uso
1. Configurar o FIWARE no Postman.
2. Provisionar o device.
3. Criar as subscriptions.
4. Iniciar a simulação no Wokwi.
5. Alterar os valores dos sensores para gerar medições diferentes.
6. Consultar Orion e STH-Comet.
7. Visualizar no dashboard.

---

## Instalação do FIWARE na máquina virtual

### Acesso à VM
No computador local, acesse a pasta onde está a chave `.pem` e conecte-se via SSH:

## Licença
Este projeto é de uso acadêmico e livre para fins educacionais, podendo ser reutilizado com os devidos créditos aos autores e à FIAP.



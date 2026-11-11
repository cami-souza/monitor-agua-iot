# Monitor IoT de Qualidade da Água para Consumo Humano

Monitor IoT de qualidade da água com ESP32 (pH, turbidez e temperatura), alertas locais por LED e buzzer e comunicação via MQTT. Projeto alinhado ao ODS 3 (Saúde e Bem-Estar), meta 3.9, desenvolvido para a disciplina de Objetos Inteligentes Conectados (Universidade Presbiteriana Mackenzie).

## Sumário

- [Contexto e objetivo](#contexto-e-objetivo)
- [Como funciona](#como-funciona)
- [Componentes](#componentes)
- [Ligações (pinagem)](#ligações-pinagem)
- [Comunicação MQTT](#comunicação-mqtt)
- [Limites de segurança](#limites-de-segurança)
- [Como executar no Wokwi](#como-executar-no-wokwi)
- [Estrutura do repositório](#estrutura-do-repositório)
- [Limitações](#limitações)
- [Autoria](#autoria)

## Contexto e objetivo
A exposição a águas com qualidade comprometida representa sério risco à saúde pública, seja pela presença de contaminantes químicos lixiviados, pela ineficiência dos processos de desinfecção ou pelo lançamento clandestino de efluentes. A Meta 3.9 do ODS 3 estabelece a redução substancial de mortes e doenças decorrentes da contaminação do ar, do solo e da água por poluentes e substâncias perigosas. No Brasil, os padrões de potabilidade para o consumo humano são regidos pela Portaria GM/MS nº 888/2021.

Neste contexto, o objetivo deste projeto é desenvolver um protótipo IoT baseado em ESP32 para o monitoramento contínuo dos parâmetros físico-químicos de pH, turbidez e temperatura da água, que são indicadores primários de eficiência de saneamento. 
O sistema atua na emissão de alertas (LED e buzzer) em cenários de não conformidade com as faixas de segurança normativas, além de realizar a telemetria dos dados via protocolo MQTT para um broker remoto, viabilizando o acompanhamento à distância e a tomada de decisão preventiva.

## Como funciona
> O projeto é desenvolvido e testado no simulador [Wokwi](https://wokwi.com). No Wokwi, os sensores de pH e turbidez são representados por potenciômetros.

1. O ESP32 conecta-se ao Wi-Fi e ao broker MQTT e assina o tópico de comandos.
2. A cada 5 segundos, lê os sensores (média de 10 amostras nos pinos analógicos, para reduzir ruído).
3. Compara os valores com os limites de segurança.
4. Aciona os atuadores:
   - **Água segura:** LED verde aceso, LED vermelho e buzzer desligados.
   - **Água imprópria:** LED vermelho aceso, LED verde apagado e buzzer tocando a 1 kHz.
5. Publica um JSON com as medições e o status no tópico de dados.
6. Um usuário remoto pode publicar `SILENCIAR` ou `ATIVAR` no tópico de comando para desligar ou restaurar o alarme sonoro.

## Componentes

| Componente no Wokwi | Componente real (protótipo físico) | Função |
|---|---|---|
| `board-esp32-devkit-c-v4` | ESP32-DevKitC V4 | Leitura, decisão, Wi-Fi e MQTT |
| `wokwi-potentiometer` (pH) | Módulo PH-4502C com eletrodo de vidro | Medir o pH |
| `wokwi-potentiometer` (turbidez) | Sensor de turbidez SEN0189 (DFRobot) | Medir a turbidez |
| `wokwi-ds18b20` | DS18B20 (sonda à prova d'água) | Medir a temperatura |
| `wokwi-resistor` 4,7 kΩ | Resistor de 4,7 kΩ | Pull-up do DS18B20 |
| `wokwi-led` verde e vermelho | LEDs de 5 mm | Indicar água segura e imprópria |
| `wokwi-resistor` 220 Ω (2) | Resistores de 220 Ω | Limitar a corrente dos LEDs |
| `wokwi-buzzer` | Buzzer piezoelétrico (passivo) | Alarme sonoro |

No protótipo físico, os módulos de pH e turbidez têm saída de até 5 V e exigem divisor de tensão antes da entrada do ESP32 (3,3 V), além de calibração com soluções de referência.

## Ligações (pinagem)

| Componente | Pino do ESP32 |
|---|---|
| Sensor de pH (SIG) | GPIO34 (ADC1) |
| Sensor de turbidez (SIG) | GPIO35 (ADC1) |
| DS18B20 (DQ) | GPIO4, com pull-up de 4,7 kΩ a 3V3 |
| LED verde (via 220 Ω) | GPIO26 |
| LED vermelho (via 220 Ω) | GPIO27 |
| Buzzer | GPIO25 |

Os terminais de alimentação dos sensores vão a 3V3 e GND; os cátodos dos LEDs e o terminal negativo do buzzer compartilham o GND.

## Comunicação MQTT

- Broker: HiveMQ (`www.mqtt-dashboard.com/`, porta `1883`)
- Tópico de dados: `ods3/qualidade_agua/camila_souza_2026/dados`
- Tópico de comando: `ods3/qualidade_agua/camila_souza_2026/comando`

Exemplo de mensagem de dados:

```json
{"ph":0.00,"turbidez":0.00,"temperatura":22.0,"status":"IMPROPRIA","alarme_silenciado":false}
{"ph":5.34,"turbidez":16.86,"temperatura":22.0,"status":"IMPROPRIA","alarme_silenciado":false}
```

Comandos aceitos no tópico de comando:

| Mensagem | Efeito |
|---|---|
| `SILENCIAR` | Desliga o buzzer; o LED vermelho continua aceso |
| `ATIVAR` | Restaura o alarme sonoro |

## Limites de segurança

| Parâmetro | Faixa considerada segura | Origem |
|---|---|---|
| pH | 6,0 a 9,0 | Faixa recomendada de 6,0 a 9,5 pela Portaria GM/MS nº 888/2021, com margem |
| Turbidez | até 5 NTU | Limite de 5,0 uT na distribuição (Portaria GM/MS nº 888/2021) |
| Temperatura | até 30 °C | Critério operacional do projeto |

## Como executar no Wokwi

1. Acesse o projeto do monitor de água no Wokwi: https://wokwi.com/projects/480346024879812609
2. Clique em Play. Gire os potenciômetros e ajuste a temperatura do DS18B20 para simular diferentes amostras de água.
3. Para acompanhar as mensagens, abra o HiveMQ Web Client, conecte-se e assine `ods3/qualidade_agua/camila_souza_2026/#`.

## Estrutura do repositório

```
.
├── README.md
├── src/
│   ├── sketch.ino        # código do ESP32 (C++ / Arduino)
│   ├── diagram.json      # circuito do Wokwi
│   └── libraries.txt     # bibliotecas
├── docs/
    └── artigo.docx       # artigo do projeto
```

## Limitações

- O broker público não oferece autenticação nem criptografia; 
- O dispositivo não mede parâmetros microbiológicos nem cloro residual. O monitoramento de pH, turbidez e temperatura **não é suficiente, por si só, para atestar a potabilidade** da água e serve como apoio à inspeção da qualidade, não devendo ser utilizado de forma autônoma como indicador de potabilidade.

## Autoria

- **Camila da Silva Souza**: desenvolvimento
- **Andre Luis de Oliveira**: professor tutor

Universidade Presbiteriana Mackenzie, Faculdade de Computação e Informática. Disciplina: Objetos Inteligentes Conectados.

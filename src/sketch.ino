#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Wi-Fi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// MQTT 
const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* ID_PROJETO  = "camila_souza_2026";
const char* TOPIC_DADOS = "ods3/qualidade_agua/camila_souza_2026/dados";
const char* TOPIC_CMD   = "ods3/qualidade_agua/camila_souza_2026/comando";

// Pinos 
const int PIN_PH     = 34;  // potenciômetro (pH)
const int PIN_TURB   = 35;  // potenciômetro (turbidez)
const int PIN_TEMP   = 4;   // DS18B20 (temp)
const int LED_VERDE  = 26;
const int LED_VERM   = 27;
const int PIN_BUZZER = 25;

// Limites de segurança
const float PH_MIN   = 6.0;
const float PH_MAX   = 9.0;
const float TURB_MAX = 5.0;    // NTU
const float TEMP_MIN = 0.0;    // graus Celsius 
const float TEMP_MAX = 30.0;   // graus Celsius

OneWire oneWire(PIN_TEMP);
DallasTemperature sensorTemp(&oneWire);
WiFiClient espClient;
PubSubClient client(espClient);

bool alerta = false;             
bool alarmeSilenciado = false;
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO = 5000;

// Aplica o estado atual aos LEDs e ao buzzer
void atualizaAtuadores() {
  digitalWrite(LED_VERDE, alerta ? LOW : HIGH);
  digitalWrite(LED_VERM,  alerta ? HIGH : LOW);
  if (alerta && !alarmeSilenciado) tone(PIN_BUZZER, 1000);
  else noTone(PIN_BUZZER);
}

// Chega uma mensagem no tópico de comando
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  msg.toUpperCase();
  Serial.print("Comando recebido: ");
  Serial.println(msg);

  if (msg == "SILENCIAR") alarmeSilenciado = true;
  if (msg == "ATIVAR")    alarmeSilenciado = false;

  atualizaAtuadores(); 
}

void conectaWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - inicio > 10000) {
      Serial.println(" tentando de novo...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
      inicio = millis();
    }
  }
  Serial.println(" conectado!");
}

void conectaMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    // ID único a cada execução, evita conflito com outros usuários do broker público
    String clientId = String("esp32-agua-") + ID_PROJETO + "-" + String((uint32_t)esp_random(), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" ok!");
      client.subscribe(TOPIC_CMD);
    } else {
      Serial.print(" falhou, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// Média de 10 leituras para reduzir ruído
float leMedia(int pino) {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(pino);
    delay(5);
  }
  return soma / 10.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERM, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  sensorTemp.begin();
  atualizaAtuadores();

  conectaWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectaWiFi();
  if (!client.connected()) conectaMQTT();
  client.loop();

  if (millis() - ultimoEnvio >= INTERVALO) {
    ultimoEnvio = millis();

    // Leitura dos sensores
    float ph   = (leMedia(PIN_PH) / 4095.0) * 14.0;    // 0 a 14
    float turb = (leMedia(PIN_TURB) / 4095.0) * 50.0;  // 0 a 50 NTU
    sensorTemp.requestTemperatures();
    float temp = sensorTemp.getTempCByIndex(0);

    // Análise
    bool tempInvalida = (temp == DEVICE_DISCONNECTED_C) || isnan(temp);  // sensor com falha
    if (tempInvalida) Serial.println("Aviso: falha na leitura do DS18B20");

    bool phRuim   = (ph < PH_MIN || ph > PH_MAX);
    bool turbRuim = (turb > TURB_MAX);
    bool tempRuim = tempInvalida || (temp < TEMP_MIN || temp > TEMP_MAX);
    alerta = phRuim || turbRuim || tempRuim;

    // Atuadores
    atualizaAtuadores();

    // Publicação MQTT (JSON)
    char payload[200];
    snprintf(payload, sizeof(payload),
      "{\"ph\":%.2f,\"turbidez\":%.2f,\"temperatura\":%.1f,"
      "\"status\":\"%s\",\"alarme_silenciado\":%s}",
      ph, turb, temp,
      alerta ? "IMPROPRIA" : "SEGURA",
      alarmeSilenciado ? "true" : "false");
    client.publish(TOPIC_DADOS, payload);

    Serial.println(payload);
  }
}

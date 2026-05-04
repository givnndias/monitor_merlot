//Turma: 1ESPA
//Equipe: Giovanna Oliveira Ferreira Dias - RM: 566647
//Maria Laura Druzeic - RM: 566634
//Marianne Mukai Nishikawa - RM:568001

//Resumo: Esse programa possibilita ligar e desligar o led onboard, além de mandar o status para o Broker MQTT possibilitando o Helix saber
//se o led está ligado ou desligado.

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// -----------------------------------------------------------------------------
// OBJETOS E PINOS
// -----------------------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço padrão do LCD (ajuste se necessário)
const int BUZZER_PIN = 27;          // Pino do buzzer
const int LDR_PIN = 34;             // Pino analógico do sensor LDR
const int DHT_PIN = 14;             // Pino do sensor DHT
#define DHTTYPE DHT22               // Tipo do sensor DHT (pode ser DHT11)
const int LED_PIN = 2;              // LED azul onboard do ESP32 (GPIO 2)

// -----------------------------------------------------------------------------
// LIMITES DE SEGURANÇA DA VINHERIA
// -----------------------------------------------------------------------------
const float TEMP_MIN = 12.0;   // Temperatura mínima ideal (°C)
const float TEMP_MAX = 18.0;   // Temperatura máxima ideal (°C)
const float HUM_MIN  = 60.0;   // Umidade mínima ideal (%)
const float HUM_MAX  = 70.0;   // Umidade máxima ideal (%)
const int   LUM_MAX  = 30;     // Luminosidade máxima permitida (%)

// -----------------------------------------------------------------------------
// CONFIGURAÇÕES DE REDE E MQTT
// -----------------------------------------------------------------------------
const char* default_SSID = "Wokwi-GUEST";    // Nome da rede Wi-Fi (Wokwi)
const char* default_PASSWORD = "";           // Senha (vazia no Wokwi)

// IP do broker MQTT (FIWARE rodando no Cloud)
const char* default_BROKER_MQTT = "34.55.148.224";
const int   default_BROKER_PORT = 1883;

// Tópicos MQTT usados pelo IoT Agent
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd";;     // Recebe comandos remotos
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs";   // Estado do LED
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l"; // Luminosidade
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp001/attrs/t"; // Temperatura
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp001/attrs/h"; // Umidade

// Identificação MQTT
const char* default_ID_MQTT = "fiware_001";
const char* topicPrefix = "lamp001"; // Prefixo para comandos

// -----------------------------------------------------------------------------
// VARIÁVEIS E OBJETOS GLOBAIS
// -----------------------------------------------------------------------------
WiFiClient espClient;       // Cliente Wi-Fi
PubSubClient MQTT(espClient); // Cliente MQTT
DHT dht(DHT_PIN, DHTTYPE);  // Objeto do sensor DHT

char EstadoSaida = '0';     // Estado atual do LED (0 = desligado, 1 = ligado)

// -----------------------------------------------------------------------------
// FUNÇÕES DE INICIALIZAÇÃO
// -----------------------------------------------------------------------------

// Inicializa o monitor serial
void initSerial() {
  Serial.begin(115200);
}

// Conecta à rede Wi-Fi
void initWiFi() {
  delay(10);
  Serial.println("------ Conexao Wi-Fi ------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(default_SSID);
  reconectWiFi();
}

// Configura o cliente MQTT
void initMQTT() {
  MQTT.setServer(default_BROKER_MQTT, default_BROKER_PORT);
  MQTT.setCallback(mqtt_callback); // Define a função que será chamada ao receber mensagens
}

// Função auxiliar para emitir beeps com o buzzer
void beepPattern(int onTime, int offTime, int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onTime);
    digitalWrite(BUZZER_PIN, LOW);
    delay(offTime);
  }
}

// -----------------------------------------------------------------------------
// FUNÇÃO DE ALERTA (verifica se há alguma anomalia)
// -----------------------------------------------------------------------------
void checkAlerts(float temp, float hum, int lum) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(temp,1);
  lcd.print("C U:");
  lcd.print(hum,0);
  lcd.print("%");

  lcd.setCursor(0,1);
  lcd.print("L:");
  lcd.print(lum);
  lcd.print("%");

  bool alerta = false; // Flag de alerta

  // Verifica temperatura
  if (temp < TEMP_MIN || temp > TEMP_MAX) {
    lcd.setCursor(9,1);
    lcd.print("TEMP!");
    beepPattern(100,100,2);   // Dois beeps curtos
    alerta = true;
  }
  // Verifica umidade
  else if (hum < HUM_MIN || hum > HUM_MAX) {
    lcd.setCursor(9,1);
    lcd.print("UMID!");
    beepPattern(100,100,3);   // Três beeps curtos
    alerta = true;
  }
  // Verifica luminosidade
  else if (lum > LUM_MAX) {
    lcd.setCursor(9,1);
    lcd.print("LUZ!");
    beepPattern(500,200,1);   // Um beep longo
    alerta = true;
  }

  // Envia comando remoto ao FIWARE (liga/desliga LED via MQTT)
  if (alerta) {
    MQTT.publish("/TEF/lamp001/cmd", "lamp001@on|");
    MQTT.publish("/TEF/lamp001/attrs/b", "on");
    Serial.println("⚠️ Alerta detectado — LED ativado remotamente!");
  } else {
    MQTT.publish("/TEF/lamp001/cmd", "lamp001@off|");
    MQTT.publish("/TEF/lamp001/attrs/b", "off");
    Serial.println("✅ Condições normais — LED desligado remotamente.");
  }
}

// -----------------------------------------------------------------------------
// CONFIGURAÇÕES INICIAIS (SETUP)
// -----------------------------------------------------------------------------
void setup() {
  InitOutput();           // Pisca o LED no início
  initSerial();           
  dht.begin();            // Inicializa o sensor DHT
  pinMode(BUZZER_PIN, OUTPUT);
  lcd.init();             // Inicializa o display LCD
  lcd.backlight();        // Liga a luz de fundo
  lcd.setCursor(0,0);
  lcd.print("Merlot Monitor");
  delay(2000);
  lcd.clear();
  initWiFi();             // Conecta ao Wi-Fi
  initMQTT();             // Configura MQTT
  delay(5000);

  // Publica estado inicial do LED no broker
  MQTT.publish(default_TOPICO_PUBLISH_1, "s|on");
}

// -----------------------------------------------------------------------------
// LOOP PRINCIPAL
// -----------------------------------------------------------------------------
void loop() {
  VerificaConexoesWiFIEMQTT(); // Garante que Wi-Fi e MQTT estejam conectados
  EnviaEstadoOutputMQTT();     // Atualiza o estado do LED no FIWARE
  handleSensors();             // Lê sensores e envia valores
  MQTT.loop();                 // Mantém o cliente MQTT ativo
}

// -----------------------------------------------------------------------------
// CONEXÃO WI-FI
// -----------------------------------------------------------------------------
void reconectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(default_SSID, default_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_PIN, LOW);
}

// -----------------------------------------------------------------------------
// CALLBACK MQTT (executada ao receber comando remoto do FIWARE)
// -----------------------------------------------------------------------------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("- Mensagem recebida: ");
  Serial.println(msg);

  String onTopic  = String(topicPrefix) + "@on|";
  String offTopic = String(topicPrefix) + "@off|";

  // Liga ou desliga o LED conforme comando recebido
  if (msg.equals(onTopic)) {
    digitalWrite(LED_PIN, HIGH);
    EstadoSaida = '1';
  }
  if (msg.equals(offTopic)) {
    digitalWrite(LED_PIN, LOW);
    EstadoSaida = '0';
  }
}

// -----------------------------------------------------------------------------
// VERIFICA CONEXÕES WI-FI E MQTT
// -----------------------------------------------------------------------------
void VerificaConexoesWiFIEMQTT() {
  if (!MQTT.connected()) reconnectMQTT();
  reconectWiFi();
}

// -----------------------------------------------------------------------------
// PUBLICA ESTADO ATUAL DO LED NO FIWARE
// -----------------------------------------------------------------------------
void EnviaEstadoOutputMQTT() {
  if (EstadoSaida == '1') {
    MQTT.publish(default_TOPICO_PUBLISH_1, "s|on");
    Serial.println("- Led Ligado");
  } else {
    MQTT.publish(default_TOPICO_PUBLISH_1, "s|off");
    Serial.println("- Led Desligado");
  }
  Serial.println("- Estado do LED enviado ao broker!");
  delay(1000);
}

// -----------------------------------------------------------------------------
// ANIMAÇÃO INICIAL DO LED AO LIGAR O SISTEMA
// -----------------------------------------------------------------------------
void InitOutput() {
  pinMode(LED_PIN, OUTPUT);
  bool toggle = false;
  for (int i = 0; i <= 10; i++) {
    toggle = !toggle;
    digitalWrite(LED_PIN, toggle);
    delay(200);
  }
  digitalWrite(LED_PIN, LOW);
}

// -----------------------------------------------------------------------------
// RECONEXÃO AO BROKER MQTT (se cair)
// -----------------------------------------------------------------------------
void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Tentando conectar ao Broker MQTT: ");
    Serial.println(default_BROKER_MQTT);

    if (MQTT.connect(default_ID_MQTT)) {
      Serial.println("Conectado ao broker MQTT!");
      MQTT.subscribe(default_TOPICO_SUBSCRIBE); // Escuta comandos remotos
    } else {
      Serial.println("Falha ao conectar. Nova tentativa em 2s...");
      delay(2000);
    }
  }
}

// -----------------------------------------------------------------------------
// LEITURA DOS SENSORES E ENVIO DOS DADOS AO FIWARE
// -----------------------------------------------------------------------------
void handleSensors() {
  // --- Luminosidade (LDR) ---
  int ldrValue = analogRead(LDR_PIN);

  // Corrige leitura invertida do LDR
  int luminosity = map(ldrValue, 0, 4095, 100, 0);
  luminosity = constrain(luminosity, 0, 100);

  String lumStr = String(luminosity);

  MQTT.publish(default_TOPICO_PUBLISH_2, lumStr.c_str());

  Serial.print("Luminosidade: ");
  Serial.print(lumStr);
  Serial.println(" %");

  // --- Temperatura e Umidade (DHT22) ---
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    String tempStr = String(temp, 1);
    String humStr  = String(hum, 1);

    MQTT.publish(default_TOPICO_PUBLISH_3, tempStr.c_str());
    MQTT.publish(default_TOPICO_PUBLISH_4, humStr.c_str());

    checkAlerts(temp, hum, luminosity);

    Serial.print(" | Temperatura: ");
    Serial.print(tempStr);
    Serial.print(" °C | Umidade: ");
    Serial.print(humStr);
    Serial.println(" %");
  } else {
    Serial.println("Falha na leitura do DHT!");
  }
}
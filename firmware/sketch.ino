#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <set>
#include <vector>
#include "secrets.h"

#define SS_PIN       5
#define RST_PIN      22
#define LED_VERDE    26
#define LED_VERMELHO 25
#define LED_STATUS   2
#define BUZZER_PIN   27

const char* TOTEM_ID = "TOTEM_QUADRA_01";

// Ajustes de robustez
const uint16_t      HTTP_TIMEOUT_MS   = 1500;
const unsigned long INTERVALO_SYNC_MS = 10000;
const size_t        MAX_FILA          = 50;
const size_t        MAX_BYTES_FLASH   = 3900;

MFRC522 rfid(SS_PIN, RST_PIN);
Preferences preferences;

std::set<String> tagsLidas;
std::vector<String> filaOffline;

unsigned long ultimoBlink = 0;
bool estadoLedStatus = false;
unsigned long ultimaSync = 0;

// ---------- Feedback (atuadores) ----------
void beep(unsigned int freq, unsigned int ms) {
  tone(BUZZER_PIN, freq, ms);
}

void feedbackLiberado() {
  digitalWrite(LED_VERDE, HIGH);
  beep(2000, 150);
  delay(150);
  beep(2500, 150);
  delay(850);
  digitalWrite(LED_VERDE, LOW);
}

void feedbackNegado() {
  digitalWrite(LED_VERMELHO, HIGH);
  beep(400, 600);
  delay(1000);
  digitalWrite(LED_VERMELHO, LOW);
}

void atualizarInterfaceRede() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_STATUS, HIGH);
  } else if (millis() - ultimoBlink >= 500) {
    ultimoBlink = millis();
    estadoLedStatus = !estadoLedStatus;
    digitalWrite(LED_STATUS, estadoLedStatus);
  }
}

void carregarTagsSalvas() {
  preferences.begin("nfc-access", true);
  String listaBruta = preferences.getString("uids", "");
  preferences.end();

  int inicio = 0;
  int fim = listaBruta.indexOf(',');
  while (fim != -1) {
    String uid = listaBruta.substring(inicio, fim);
    if (uid.length() > 0) tagsLidas.insert(uid);
    inicio = fim + 1;
    fim = listaBruta.indexOf(',', inicio);
  }
  Serial.println("Tags carregadas da flash: " + String(tagsLidas.size()));
}

void salvarTagNaFlash(const String& uid) {
  preferences.begin("nfc-access", false);
  String listaAtual = preferences.getString("uids", "");
  if (listaAtual.length() + uid.length() + 1 > MAX_BYTES_FLASH) {
    Serial.println("AVISO: flash cheia, tag mantida so na RAM");
  } else {
    listaAtual += uid + ",";
    preferences.putString("uids", listaAtual);
  }
  preferences.end();
}

String montarPayloadJSON(const String& uid, const String& status, bool offline) {
  String json = "{";
  json += "\"totem_id\":\"" + String(TOTEM_ID) + "\",";
  json += "\"uid\":\"" + uid + "\",";
  json += "\"status\":\"" + status + "\",";
  json += "\"offline\":" + String(offline ? "true" : "false") + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";
  return json;
}

bool enviarParaAPI(const String& payload) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(API_URL)) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  int codigo = http.POST(payload);
  http.end();

  return (codigo >= 200 && codigo < 300);
}

void enfileirar(const String& payload) {
  if (filaOffline.size() >= MAX_FILA) {
    filaOffline.erase(filaOffline.begin());
  }
  filaOffline.push_back(payload);
}

void sincronizarFilaOffline() {
  if (filaOffline.empty()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - ultimaSync < INTERVALO_SYNC_MS) return;
  ultimaSync = millis();

  Serial.println("Sincronizando fila offline (" + String(filaOffline.size()) + ")...");
  while (!filaOffline.empty()) {
    if (enviarParaAPI(filaOffline.front())) {
      filaOffline.erase(filaOffline.begin());
    } else {
      break;
    }
  }
}

void registrarEvento(const String& uid, const String& status) {
  if (!enviarParaAPI(montarPayloadJSON(uid, status, false))) {
    enfileirar(montarPayloadJSON(uid, status, true));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  SPI.begin();
  rfid.PCD_Init();

  carregarTagsSalvas();

  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  Serial.println("Sistema iniciado. Aproxime a tag NFC...");
}

void loop() {
  atualizarInterfaceRede();
  sincronizarFilaOffline();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uidLido = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidLido += "0";
    uidLido += String(rfid.uid.uidByte[i], HEX);
  }
  uidLido.toUpperCase();
  Serial.println("Tag detectada: " + uidLido);

  // Decisao local (R2): funciona sem internet
  if (tagsLidas.find(uidLido) != tagsLidas.end()) {
    Serial.println("DECISAO LOCAL: Acesso Negado (Duplicado)");
    feedbackNegado();
    registrarEvento(uidLido, "NEGADO_DUPLICADO");
  } else {
    Serial.println("DECISAO LOCAL: Acesso Liberado");
    tagsLidas.insert(uidLido);
    salvarTagNaFlash(uidLido);
    feedbackLiberado();
    registrarEvento(uidLido, "LIBERADO");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

/*
 * 
 * Laboratorio 8
 * Conexion de MCU
 * 
 */

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>  // Incluye la biblioteca ArduinoJSON


// Valores para conectarse a la red WiFi.
const char* ssid = "Harold.V";
const char* password = "1234567890.";

// Puertos.
const int sensorPin = A0;
const int ledPin = D2;

// topicos de salidad y entrada
const char* outTopic = "mobile/mensajes";
const char* inTopic = "sensor/command";

/*******************************************************/
// valor que se espera del topico de entrada para ajustar
// los valores de luminicencia.
int inValue = 30;
// contador para controlar los envios cada 30 sec.
unsigned long lastSendTime = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//AWS este valor lo optines del portal de AWS
const char * AWS_endpoint = "a3brjmyw4c304l-ats.iot.us-east-2.amazonaws.com"; //MQTT broker ip

void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Mensaje entrante: [");
  Serial.print(topic);
  Serial.println("] ");

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  StaticJsonDocument<200> jsonDoc;
  
  DeserializationError error = deserializeJson(jsonDoc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  /* Obtenemos el valor y lo almacenamos en una variable.*/
  String valueStr = jsonDoc["value"];  
  //String valueStr = "80";
  inValue = valueStr.toInt();
  //intValue = 100;
  Serial.println(inValue);
}

/* Se instanc */
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //MQTT port 8883 - standard
// long lastMsg = 0;
// char msg[50];
// int value = 0;

void setup_wifi() {
  delay(10);
  // Inicializamos Wifi
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}
void reconnect() {
  // Reconeccion si es necesario
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPthing_RONY")) {
      
      Serial.println("connected");
      //client.publish(outTopic, "hello world");
      client.subscribe(inTopic);

    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}


/**********************/
void getDateTime(unsigned long epochTime, char* dateTimeString) {
  // 5 es el offset para obtener la hora de acuerdo a nuesa zona horario.
  epochTime -= 5 * 3600;

  // calcular los componentes de fecha y hora
  int year, month, day, hour, minute, second;
  unsigned long days;
  unsigned long remaining;

  second = epochTime % 60;
  epochTime /= 60;
  minute = epochTime % 60;
  epochTime /= 60;
  hour = epochTime % 24;
  epochTime /= 24;
  days = epochTime;
year = 1970;
  while (true) {
    int daysInYear = 365;
    if (year % 4 == 0) {
      if (year % 100 != 0 || year % 400 == 0) {
        daysInYear = 366; // Es un año bisiesto
      }
    }

    if (days < daysInYear) {
      break;
    }
    days -= daysInYear;
    year++;
  }

  for (month = 1; month <= 12; month++) {
    int monthLength;
    if (month == 2) {
      if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        monthLength = 29; // Febrero en año bisiesto
      } else {
        monthLength = 28; // Febrero en año no bisiesto
      }
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
      monthLength = 30; // Meses con 30 días
    } else {
      monthLength = 31; // Meses con 31 días
    }

    if (days < monthLength) {
      break;
    }
    days -= monthLength;
  }
  day = days + 1; // Sumar 1 para ajustar el día al valor correcto

  // Formatear la fecha y hora en una cadena
  sprintf(dateTimeString, "%02d-%02d-%04d %02d:%02d:%02d", month, day, year, hour, minute, second);
  
}



void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  // inicializamos el led del esp8266
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
  // Cargamos los certificados
  File cert = SPIFFS.open("/cert.der", "r"); //verifica
  if (!cert) {
    Serial.println("Failed to open cert file");
  } else
    Serial.println("Success to open cert file");
  delay(1000);
  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");
  // Cargamos llave privada
  File private_key = SPIFFS.open("/private.der", "r"); 
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  } else
    Serial.println("Success to open private cert file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
  // Cargamos CA 
  File ca = SPIFFS.open("/ca.der", "r"); 
  if (!ca) {
    Serial.println("Failed to open ca ");
  } else
    Serial.println("Success to open ca");
  delay(1000);
  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  timeClient.update();
  int lightSensor = analogRead(sensorPin);
  StaticJsonDocument<200> jsonDoc;
  unsigned long epochTime = timeClient.getEpochTime();
  /* Cada 30 segundos envimaos un valor al topico*/
  /* Obtenemose el valor del sensor y creamos el documento json para enviarlo. */
  char formattedTime[30];
  getDateTime(epochTime, formattedTime);
  
  jsonDoc["Timestamp"] = formattedTime;
  jsonDoc["Value"] = lightSensor;
  jsonDoc["Unit"] = "Lx";
  jsonDoc["Notes"] = "TEST";

  // Serealizar el objeto JSON
  String jsonStr;
  serializeJson(jsonDoc, jsonStr);
  // empleammos analogWrite para enviar el valor que recibimos del servicio.
  analogWrite(ledPin, inValue);

  if ( millis() - lastSendTime > 30000 ) {
    Serial.println("Mensaje Enviado: [mobile/mensajes]");
    Serial.println(jsonStr.c_str());
    client.publish(outTopic, jsonStr.c_str());  
    lastSendTime = millis();
  }

  digitalWrite(LED_BUILTIN, HIGH); 
  delay(1000); // esperar por 1 sec
  digitalWrite(LED_BUILTIN, LOW);
}

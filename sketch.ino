#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <SPI.h>

#include <WiFi.h>
#include <PubSubClient.h>

// photoresistor configuration (brightness)
#define LDR 5

// potentiometer configuration (assume this is moisture checker)
#define POTEN 6

// temperature configuration (temperature)
#define TEMP 7

// ili9341 configuration (screen)
#define MOSI 11
#define SCK 12
#define MISO 13
#define CS 10
#define DC 9
#define RST 4

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topic = "send/data";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_ILI9341 tft = Adafruit_ILI9341(CS, DC, RST);

GFXcanvas16 canvas(320, 240);

void setup(){
  tft.begin();
  canvas.setTextSize(2);
  canvas.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    canvas.print(".");
    tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 320, 240);
  }

  canvas.fillScreen(ILI9341_BLACK);
  canvas.setCursor(0, 0);
  canvas.println("WiFi Connected !");
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 320, 240);

  client.setServer(mqtt_server, mqtt_port);
}

void loop(){
  if (!client.connected()){
    String clientId = "ESP32-Ahu" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())){
      canvas.fillScreen(ILI9341_BLACK);
      canvas.setCursor(0, 0);
      canvas.println("MQTT Connected !");
      tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 320, 240);
    }

    else{
      canvas.fillScreen(ILI9341_BLACK);
      canvas.setCursor(0, 0);
      canvas.print("MQTT Error. status=");
      canvas.println(client.state());
      tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 320, 240);
      delay(2000);
    }
  }

  client.loop();

  
  int poten_value = analogRead(POTEN);


  const float BETA = 3950;
  int temp_raw = analogRead(TEMP);
  int temp_value = map(temp_raw, 0, 4095, 0, 1023);
  float celsius = 1 / (log(1 / (1023. / temp_value - 1)) / BETA + 1.0 / 298.15) - 273.15;


  const float GAMMA = 0.7;
  const float RL10 = 50;

  int ldr_value = analogRead(LDR);
  float voltage = ldr_value * 3.3 / 4095.0;

  float R_fixed = 2000.0;
  float resistance = R_fixed * voltage / (3.3 - voltage);

  const float GAMMA = 0.7;
  const float RL10 = 50.0;

  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, 1.0 / GAMMA);


  char data[100];
  sprintf(data, "{\"moisture\": %d,\"temperature\": %2.f,\"brightness\": %d}", poten_value, celsius, lux);
  client.publish(topic, data);


  canvas.fillScreen(ILI9341_BLACK);
  canvas.setCursor(0, 0);
  canvas.print("MOISTURE: ");
  canvas.println(poten_value);


  canvas.setCursor(0, 30);
  canvas.print("TEMPERATURE: ");
  canvas.print(celsius);
  canvas.println(" C");


  canvas.setCursor(0, 60);
  canvas.print("BRIGHTNESS: ");
  canvas.print(lux);
  canvas.println(" lux");

  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 320, 240);
  delay(1000);
}

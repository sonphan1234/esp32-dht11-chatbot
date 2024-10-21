#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Thay đổi địa chỉ I2C nếu cần thiết

// WiFi credentials
char ssid[] = "Sonphan";
char password[] = "12345678";

// Telegram BOT
#define BOTtoken "6541745731:AAHPLv0jl6y4tSy-l4qVZD7qWW2d2nDCkkE"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 500; // Check bot messages every 0.5 seconds
unsigned long lastTimeBotRan = 0;

// DHT sensor readings
int temperature = 0;
int humidity = 0;

String chat_id = "6791132803"; // Your chat ID to send alerts

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    Serial.print("Message received from: ");
    Serial.println(from_name);
    Serial.print("Chat ID: ");
    Serial.println(chat_id);
    Serial.print("Text: ");
    Serial.println(text);

    if (chat_id == "6791132803") {
      if (text == "/start") {
        String welcome = "Welcome " + from_name + "\n\nThis bot is Built by Trung to find room temperature and humidity.\n\nChoose your option:\n";
        welcome += "/status : Get Temperature and Humidity\n";
        Serial.println("Sending welcome message:");
        Serial.println(welcome);
        bot.sendMessage(chat_id, welcome, "Markdown");
      }

      if (text == "/status") {
        String status = "Temperature: " + String(temperature) + "°C\n";
        status += "Humidity: " + String(humidity) + "%\n";

        if (temperature > 30 || humidity > 70) {
          status += "\n!Warning!!!\n";
          status += "The temperature and/or humidity levels are at a dangerous level. Please be careful when going outside!";
        }

        status += "\nBuilt by Trung";
        Serial.println("Sending status:");
        Serial.println(status);
        bot.sendMessage(chat_id, status, "");
      }
    } else {
      bot.sendMessage(chat_id, "Unauthorized User", "");
    }
  }
}

void connectToWiFi(void *parameter) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  
  int retryCount = 0;
  const int maxRetries = 20;
  
  while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
    Serial.print(".");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    retryCount++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
  }

  vTaskDelete(NULL); // Delete this task once WiFi is connected
}

void handleBot(void *parameter) {
  for (;;) {
    if (millis() - lastTimeBotRan > botRequestDelay) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages) {
        Serial.println("Got Response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      lastTimeBotRan = millis();
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Delay 0.1 second between each loop iteration
  }
}

void readDHTSensor(void *parameter) {
  for (;;) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);

    // Update LCD display
       if (temperature > 30 || humidity > 70) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp: " + String(temperature) + "C ");
      lcd.setCursor(0, 1);
      lcd.print("Hum: " + String(humidity) + "%  ");
      vTaskDelay(2000 / portTICK_PERIOD_MS);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("The temp, hum levels");
      lcd.setCursor(0, 1);
      lcd.print("are at a dangerous level.");
      vTaskDelay(2000 / portTICK_PERIOD_MS);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Please be careful");
      lcd.setCursor(0, 1);
      lcd.print("when going outside!");
      vTaskDelay(2000 / portTICK_PERIOD_MS);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Built by Sonphan");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp: " + String(temperature) + "C ");
      lcd.setCursor(0, 1);
      lcd.print("Hum: " + String(humidity) + "%  ");
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }
}

void checkWiFiStatus(void *parameter) {
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected");
    } else {
      Serial.println("WiFi connected");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Check WiFi status every second
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  client.setInsecure();

  Wire.begin(18, 19); // Set I2C pins: SDA = 18, SCL = 19
  lcd.init();
  lcd.backlight();

  // Create tasks
  xTaskCreate(connectToWiFi, "WiFiTask", 4096, NULL, 1, NULL);
  xTaskCreate(handleBot, "BotTask", 8192, NULL, 1, NULL);
  xTaskCreate(readDHTSensor, "DHTTask", 2048, NULL, 1, NULL);
  xTaskCreate(checkWiFiStatus, "WiFiStatusTask", 2048, NULL, 1, NULL);
}

void loop() {
  // Empty, tasks are now managed by FreeRTOS
}

#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h> // Include the Web Server library

// Network credentials
const char* ssid = "NETWORK";
const char* password = "PASSWORD";

// MQTT Broker settings
const char* mqtt_server = "192.168.1.10"; // Replace with the IP of your MQTT broker
const int mqtt_port = 1883;
const char* front_distance_topic = "robot/sensor/front";
const char* rear_distance_topic = "robot/sensor/rear";

// Ultrasonic Sensor Front Pins
const int triggerPinFront = 19; // GPIO 19 for front trigger
const int echoPinFront = 18;    // GPIO 18 for front echo

// Ultrasonic Sensor Rear Pins
const int triggerPinRear = 22;  // GPIO 22 for rear trigger
const int echoPinRear = 23;     // GPIO 23 for rear echo

// Variables to store the duration and the distance
long durationFront, durationRear;
float distanceInchFront, distanceInchRear;

// Initialize the WiFi and MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

WebServer server(80); // Initialize the WebServer object on port 80

unsigned long previousMillis = 0; // Will store last time the distance was updated
const long interval = 200;        // Interval at which to read sensor (milliseconds)

void setup() {
  // Start the Serial Communication
  Serial.begin(115200);
  
  // Set sensor pins as output and input for front and rear sensor
  pinMode(triggerPinFront, OUTPUT);
  pinMode(echoPinFront, INPUT);
  pinMode(triggerPinRear, OUTPUT);
  pinMode(echoPinRear, INPUT);

  // Connect to the Wi-Fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Once connected, print the IP address
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to the MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  
  // Setup routes for your web server
  server.on("/", handleRoot);
  server.on("/front", handleDistanceFront);
  server.on("/rear", handleDistanceRear);

  server.begin(); // Start the web server
}
bool mqttInitialConnect = false;

void loop() {
  unsigned long currentMillis = millis();

  // Handle WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    WiFi.reconnect();  // Attempt to reconnect to WiFi
  }

  // Proceed with sensor readings regardless of MQTT connection
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    measureDistanceFront(); // Measure front distance
    measureDistanceRear();  // Measure rear distance

    // Print the distances to the Serial Monitor
    Serial.print("Front Distance: ");
    Serial.print(distanceInchFront);
    Serial.println(" inches");
    Serial.print("Rear Distance: ");
    Serial.print(distanceInchRear);
    Serial.println(" inches");
  }

  // MQTT Connection handling
  if (!client.connected()) {
    Serial.println("MQTT not connected, attempting to connect...");
    reconnectMQTT();
  } else {
    client.loop();  // Normal operation of MQTT client
  }

  // Handle web server
  server.handleClient();
}

int mqttAttemptCounter = 0; // Keep track of how many times we've tried
const int mqttMaxAttempts = 5; // Max number of times to print attempts
bool mqttQuietMode = false; // Flag to turn off prints after max attempts

void reconnectMQTT() {
  if (!mqttInitialConnect && mqtt_server != "") {
    // Only try to connect if we haven't connected before
    if (!mqttQuietMode) { // If we're not in quiet mode, print the message
      Serial.print("Attempting MQTT connection...");
    }
    
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      mqttInitialConnect = true; // Set the flag here
      // Subscribe to topics or perform other MQTT setup here if needed
    } else {
      if (!mqttQuietMode) { // If we're not in quiet mode, print the error
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" will enter quiet mode after a few more attempts...");
      }
      mqttAttemptCounter++; // Increment the counter
      if (mqttAttemptCounter >= mqttMaxAttempts) {
        // If we've reached the max attempts, enable quiet mode
        mqttQuietMode = true;
      }
    }
  }
}
void measureDistanceFront() {
  Serial.println("Measuring Front Distance...");
  // Clears the triggerPin condition
  digitalWrite(triggerPinFront, LOW);
  delayMicroseconds(2);

  // Sets the triggerPin high (for set period of time)
  digitalWrite(triggerPinFront, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPinFront, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  durationFront = pulseIn(echoPinFront, HIGH);

  // Calculating the distance in inches
  distanceInchFront = (durationFront * 0.034 / 2) / 2.54;
  Serial.print("Front Distance Measured: ");
  Serial.println(distanceInchFront);
}

void measureDistanceRear() {
  Serial.println("Measuring Rear Distance...");

  // Clears the triggerPin condition
  digitalWrite(triggerPinRear, LOW);
  delayMicroseconds(2);
  
  // Sets the triggerPin high (for set period of time)
  digitalWrite(triggerPinRear, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPinRear, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  durationRear = pulseIn(echoPinRear, HIGH);
  
  // Calculating the distance in inches
  distanceInchRear = (durationRear * 0.034 / 2) / 2.54;
}


void handleRoot() {
  String html = "<h1>ESP32 Ultrasonic Sensor Readings</h1>"
                "<p><a href=\"/front\">Front Distance</a></p>"
                "<p><a href=\"/rear\">Rear Distance</a></p>";
  server.send(200, "text/html", html);
}

// Front distance handler
void handleDistanceFront() {
  measureDistanceFront(); // Call the measure function for the front sensor
  server.send(200, "text/plain", String(distanceInchFront) + " inches");
}

void handleDistanceRear() {
  measureDistanceRear(); // Call the measure function for the rear sensor
  server.send(200, "text/plain", String(distanceInchRear) + " inches");
}
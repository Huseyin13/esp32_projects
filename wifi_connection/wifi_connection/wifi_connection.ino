#include <WiFi.h>

// Wi-Fi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

WiFiServer server(80);
const int ledPin = 5;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  server.begin(); // Start the web server
}

void loop() {
  WiFiClient client = server.available();

  if(client){
    Serial.println("New client connected.");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // LED on/off control
    if(request.indexOf("/LED_ON") != -1){
      digitalWrite(ledPin, HIGH);
    }
    if(request.indexOf("/LED_OFF") != -1){
      digitalWrite(ledPin, LOW);
    }

    // HTML page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();

    client.println("<html><body>");
    client.println("<h1>ESP32 LED control</h1>");
    client.println("<p><a href=\"/LED_ON\">LED On</a></p>");
    client.println("<p><a href=\"/LED_OFF\">LED Off</a></p>");
    client.println("</body></html>");

    delay(1);
    Serial.println("Client disconnected.");

  }


}

#include <WİFİ.h>

//Wi-fi bilgileri
const char* ssid = "ESP32";
const char* password = "f4i3kd5l";

WiFiServer server(80);
const int ledPin = 5;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  //Wi-fi'ye bağlanma aşaması
  Serial.print("Wi-fi ağına bağlanılıyor...");
  WiFi.begin(ssid, password);

  while(WiFi.status != WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi bağlantısı başarılı");
  Serial.print("Esp32 IP Adresi: ");
  Serial.println(WiFi.localIP());
  
  server.begin();//Burada artık web sunucusunu başlatıyoruz
}

void loop() {
  WiFi client = server.available();

  if(client){
    Serial.println("Yeni istemci bağlandı.");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    //LED ac/kapat kontrolü
    if(request.indexOf("/LED_ON") != -1){
      digitalWrite(ledPin, HIGH);
    }
    if(request.indexOf("/LED_OFF") != -1){
      digitalWrite(ledPin, LOW);
    }

    //HTML sayfası
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();

    client.println("<html><body>");
    client.println("<h1>ESP32 LED kontol</h1>");
    client.println("<p><a href=\"/LED_ON\">LED Aç</a><p>");
    client.println("<p><a href=\"/LED_OFF\">LED Kapat</a></p>");
    client.println("</body></html>");

    delay(1);
    Serial.println("İstemci bağlantısı sonlandı.");

  }
  

}

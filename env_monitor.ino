#include <sys/socket.h>
#include <WiFi.h>
#include "time.h"
#include "DHT.h"
#include <errno.h>

#define DHTPIN 19
#define DHTTYPE DHT22


// Wifi settings
const char *ssid = "YOURSSID";
const char *pass = "YOURPSK";


// Clock settings
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Graphite settings
const char *graphite_server = "192.168.1.20";
const char *metric_base = "home.environmental";
const char *my_name = "garage";
int graphite_port = 2003;



// Dallas one-wire
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("Beginning program\n");
  dht.begin();

  Serial.print("Connecting to wifi: ");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.printf("IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void printLocalTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Error: Failed to obtain time\n");
        return ;
    }

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.println();
}

float ctof(float c) {
  return ((c * (9.0/5.0)) + 32.0);
}

void print_status(float humidity, float temp) {
  Serial.printf("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("Humidity: %f\n", humidity);
  Serial.printf("Temperature: %fC (%fF)\n", temp, ctof(temp));
  printLocalTime();
  Serial.println("---\n");
}

int send_metrics(float h, float t, const char *server, int port) {
    char rembuf[256];
    char locbuf[256];
    int j;

    time_t now;
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo)) {
        Serial.println("Error: failed to obtain time\n");
        return -1;
    }
    time(&now);

    // TCP socket vars
    int sock = 0;
    int valread;
    struct sockaddr_in serv_addr;
    char tcpbuf[1024] = {0};
    
    // buf is the text to send
    j = snprintf(rembuf, 256, "%s.%s.temp_c %.2f %lu\n%s.%s.humidity %.2f %lu\n%s.%s.temp_f %.2f %lu\n", 
        metric_base, 
        my_name, 
        t,
        now,
        metric_base, 
        my_name, 
        h,
        now,
        metric_base,
        my_name,
        ctof(t),
        now);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        Serial.println("\nSocket creation error\n");
        return -1;
    }
    Serial.printf("Got socket %x\n", sock);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server, &serv_addr.sin_addr) <= 0) {
        Serial.printf("Error: Invalid address %s", server);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        Serial.printf("%i", serv_addr.sin_addr.s_addr);
        Serial.printf("Error: Connection failed: %s", strerror(errno));
        return -1;
    }
    j = snprintf(locbuf, 256, "Connected to server %s on port %i", server, port);
    Serial.println(locbuf);

    send(sock, rembuf, strlen(rembuf), 0);
    j = snprintf(locbuf, 256, "Sent data:\n%s\n", rembuf); 
    close(sock);
    Serial.println(locbuf);
    //valread = read(sock, rembuf, 1024);
    //Serial.printf("Got back %x\n", valread);

    return 0;
}

void loop() {
  delay(10000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to reach DHT sensor!");
  } else {
    print_status(h, t);
    send_metrics(h, t, graphite_server, graphite_port);
  }
  
}

/****************************************************************************
 * PiEM-ESP8266 Modul                                                       *
 * ==================                                                       *
 * Dieser Sketch für den ESP8266 dient als remote Zählersensor für PiEM-It! *
 * und benötigt folgende Libraries:                                         *
 *  WiFi, NTP, Time                                                         *
 *                                                                          *
 * Die Übertragung des Zählimpuls erfolgt per HTTP-Get Request an das       *
 * Webserver Modul von PiEM-It!                                             *
 *                                                                          *
 * Homepage: http://piem.TGD-Consulting.de                                  *
 *                                                                          *
 * Version 0.1.0                                                            *
 * Datum 07.12.2016                                                         *
 *                                                                          *
 * (C) 2016 TGD-Consulting , Author: Dirk Weyand                            *
 ****************************************************************************/ 

/*************************
 *** Globale Parameter ***
 *************************/

#define WLAN_SSID               "SSID des WLAN"          // change to your WiFi SSID 
#define WLAN_PASSPHRASE         "DAS GEHEIME PASSWORT"   // change to your passphrase
#define NTP_SERVER              "192.168.9.28"           // set your NTP-Server here, eg. "us.pool.ntp.org"
#define PIEM_HOST               "192.168.9.25"           // PiEM-It! Webserver
#define PIEM_PORT               8080                     // Port des Webservers
#define ZAEHLER_ID              "123456789"              // eindeutige ID des Z‰hlersensors
#define TOKEN                   "000000453c67f0"         // Verbindungstoken (Seriennummer des RPi)
#define PST +1           // MESZ
#define SERDEBUG 1       // Debug-Infos ¸ber Serielle Schnittstelle senden, bei 0 Debugging OFF  
#define GPIO_INPUT 3     // Rx wird als Input-GPIO verwendet

// include requiered library header
#include <ntp.h>
#include <ESP8266WiFi.h> // WiFi functionality
#include <WiFiUdp.h>     // udp for network time
#include <Time.h>

// function pre declaration 2 avoid errors
bool startWiFi(void);
time_t getNTPtime(void);

NTP NTPclient;

void setup() {
#ifdef SERDEBUG
   Serial.begin(115200);
   delay(10);
   Serial.println();
   Serial.println();
   Serial.println("PiEM-ESP8266");
   Serial.println(ZAEHLER_ID);
#endif

   // mit WLAN-AP verbinden
   while (!startWiFi()){delay(1500);}

#ifdef SERDEBUG  
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
#endif

   NTPclient.begin(NTP_SERVER, PST);
   setSyncInterval(SECS_PER_HOUR);
   setSyncProvider(getNTPtime);

   pinMode(GPIO_INPUT, INPUT_PULLUP); 

   delay(1000);  // nach dem Start 1 Sekunden Zeit, für NTP-Synchronisation

}

void loop() {
    // Wait for Z‰hlimpuls
    while (digitalRead(GPIO_INPUT) == HIGH) // While GPIO_INPUT pin is HIGH (not activated)
        yield();                            // Do (almost) nothing -- yield to allow ESP8266 background functions

    // Zählimpuls erkannt => Signalisierung an PIEM-Server
    string DateTimeString = String(now.day(),DEC) + "-" + String(now.month(),DEC) + "-" + String(now.year(),DEC);
    DateTimeString= DateTimeString + "/" + String(now.hour(),DEC) + ":"+String(now.minute(),DEC) + ":"+String(now.second(),DEC);

#ifdef SERDEBUG
    Serial.print("connecting to ");
    Serial.println(PIEM_HOST);
#endif

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    if (!client.connect(PIEM_HOST, PIEM_Port)) {
#ifdef SERDEBUG
       Serial.println("connection failed");
#endif
       return;
    }

    // We now create a URI for the request
    String url = "/cgi-bin/import.html?id=";
    url += Zaehler_ID;
    url += "&token=";
    url += TOKEN;
    if (timeStatus() != timeNotSet){ // Falls Zeit synchron zum NTP-Server, Zeitpunkt ¸bermitteln
       url += "&time=";
       url += DateTimeString;        // im REBOL Time-Format
    }

#ifdef SERDEBUG  
    Serial.print("Requesting URL: ");
    Serial.println(url);
#endif
  
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + PIEM_HOST + "\r\n" +
                "Connection: close\r\n\r\n");
    delay(100); // Wartezeit bevor erneut auf neuen Z‰hlimpuls gewartet wird

}

#define NTP_RETRIES 3 // Anzahl Versuche, die Uhrzeit vom NTP zu bekommen

time_t getNTPtime(void)
{
   time_t retVal = 0;

   for( int i = 0; i < NTP_RETRIES && retVal == 0; i++ )
   {
     retVal = NTPclient.getNtpTime();
   }
   return( retVal );
}

bool startWiFi(void)
{
   uint8_t i;

#ifdef SERDEBUG
   Serial.print("Attempting to Connect to ");
   Serial.print(WLAN_SSID);
   Serial.print(" using password ");
   Serial.println(WLAN_PASSPHRASE);
#endif

   if (WiFi.begin(WLAN_SSID, WLAN_PASSPHRASE) != WL_CONNECTED) {
      for (i=0;i<10;i++){
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(500);
#ifdef SERDEBUG
        Serial.print(".");
#endif
      }
   }

#ifdef SERDEBUG
   Serial.print("Failed to connect to: ");
   Serial.println(WLAN_SSID);
  
   Serial.print("using pass phrase: ");
   Serial.println(WLAN_PASSPHRASE);
#endif

   return false;
}

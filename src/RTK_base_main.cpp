// Copy credentials_sample.h to credentials.h and fill in your credentials.
#include "credentials.h" // Contains secrets like WiFi password and NTRIP caster credentials.

#include <Arduino.h>
//#include <ESP8266WiFi.h>  //Need for ESP8266
#include <WiFi.h>           //Need for ESP32 
#include <HardwareSerial.h>
#include <NTRIPServer.h> // Forked from https://github.com/GLAY-AK2/NTRIP-server-for-Arduino.git

#define DEBUG false

int restart_counter = 10; // Counter to keep track of WiFi reconnects. If WiFi is not connected after 10 tries, the ESP32 will restart.
uint8_t byte_buffer[2048]; // buffer for reading the RtcmSerial port
int byte_count = 0; // counter to keep track of buffer length
int messages_in_buffer = 0; // counter to keep track of messages in buffer

const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// stdup used in setup() because of the following warning:
// warning: ISO C++ forbids converting a string constant to 'char*' [-Wwrite-strings]
#ifdef HOST_1
  char* host_1;
  int httpPort_1;
  char* mntpnt_1;
  char* psw_1;
  char* srcSTR_1;
#endif
#ifdef HOST_2
  char* host_2;
  int httpPort_2;
  char* mntpnt_2;
  char* psw_2;
  char* srcSTR_2;
#endif

NTRIPServer ntrip_server_1;
NTRIPServer ntrip_server_2;
HardwareSerial & RtcmSerial = Serial2;

const uint8_t newline_byte = byte('\n');

void reset_counters_buffers() {
  // reset the counters and whipe the buffer
  
  restart_counter = 10;
  messages_in_buffer = 0;
  byte_count = 0; 
  for (unsigned int i = 0; i < sizeof(byte_buffer); i++){
    byte_buffer[i] = 0x00; // wipe the buffer for the next loop
  }
} // end of reset_counters_buffers()

void send_buffer_to_ntrip_server(char* host, int httpPort, char* mntpnt, char* psw, char* srcSTR, NTRIPServer ntrip_server) {
  // send the buffer to the NTRIP server

  if (ntrip_server.connected()) {
    ntrip_server.write(byte_buffer, byte_count); 
    if (DEBUG){Serial.print(byte_count); Serial.println(F(" packet sent."));}
  } else { // if we are not connected to the NTRIP server, try to reconnect
    ntrip_server.stop();
    Serial.println(host);
    Serial.println("reconnect");
    Serial.println("Subscribing MountPoint");
    if (!ntrip_server.subStation(host, httpPort, mntpnt, psw, srcSTR)) {
      delay(100);
    } else {
      Serial.println("Subscribing MountPoint is OK");
      delay(10);
    }
  } // end of if ntrip_s.connected()
} // end of send_buffer_to_ntrip_server()

void reconnect_wifi() {
  // reconnect to WiFi
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi: ");
    Serial.print(ssid);
    Serial.print(" : ");
    Serial.println(password);
    
    while (WiFi.status() != WL_CONNECTED) {
      // restart the device if we can't connect to WiFi after 10 tries
      delay(2000);
      Serial.print(".");
      if(restart_counter-- < 1) {
        ESP.restart();
      }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    restart_counter = 10;
    RtcmSerial.flush();
  } // end of if WiFi.status() != WL_CONNECTED
} // end of reconnect_wifi()

void read_serial() {
  // read all messages from the serial port to the buffer

  while (RtcmSerial.available()) {
    byte c = RtcmSerial.read(); // read in a byte
    byte_buffer[byte_count++] = c; // add byte to buffer and increment counter
    
    if ((c == newline_byte) || (byte_count == sizeof(byte_buffer) - 1)) {
      // if the buffer gets full or we see the newline character
      messages_in_buffer += 1;
      if (DEBUG){Serial.print(byte_count); Serial.println(F(" packet read."));}
      break;
    }
  } // end of while RtcmSerial.available()
} // end of read_serial()

void setup() {
  bool mntpnt_subscribed = false; // flag to keep track of subscription status
  
  #ifdef HOST_1
    host_1 = strdup(HOST_1);
    httpPort_1 = HTTP_PORT_1;
    mntpnt_1 = strdup(MNT_PNT_1);
    psw_1 = strdup(PSW_1);
    srcSTR_1 = strdup(SRC_STR_1);
  #endif
  #ifdef HOST_2
    host_2 = strdup(HOST_2);
    httpPort_2 = HTTP_PORT_2;
    mntpnt_2 = strdup(MNT_PNT_2);
    psw_2 = strdup(PSW_2);
    srcSTR_2 = strdup(SRC_STR_2);
  #endif

  Serial.begin(115200);
  RtcmSerial.begin(115200);
  delay(10);
  
  WiFi.begin(ssid,password);
  reconnect_wifi();

  #ifdef HOST_1
    Serial.print("Subscribing MountPoint: ");
    Serial.print(mntpnt_1);
    Serial.print(" : ");
    Serial.println(psw_1);
    Serial.println(host_1);
    if (ntrip_server_1.subStation(host_1, httpPort_1, mntpnt_1, psw_1, srcSTR_1)) {
      mntpnt_subscribed = true;
      Serial.println("Subscribing MountPoint is OK");
    }
  #endif

  #ifdef HOST_2
    Serial.print("Subscribing MountPoint: ");
    Serial.print(mntpnt_2);
    Serial.print(" : ");
    Serial.println(psw_2);
    Serial.println(host_2);
    if (ntrip_server_2.subStation(host_2, httpPort_2, mntpnt_2, psw_2, srcSTR_2)) {
      mntpnt_subscribed = true;
      Serial.println("Subscribing MountPoint is OK");
    }
  #endif

  if (!mntpnt_subscribed) {
    // if we can't subscribe to the NTRIP server, restart the device
    Serial.println("Failed to subscribe. Restarting device...");
    delay(15000);
    ESP.restart();
  }
}

void loop() {
  if ((messages_in_buffer > 0)) {
    // if there are messages in the buffer send them to the NTRIP server
    if (DEBUG){Serial.print(messages_in_buffer); Serial.println(F(" messages in buffer."));}
    #ifdef HOST_1
      send_buffer_to_ntrip_server(host_1, httpPort_1, mntpnt_1, psw_1, srcSTR_1, ntrip_server_1);
    #endif
    #ifdef HOST_2
      send_buffer_to_ntrip_server(host_2, httpPort_2, mntpnt_2, psw_2, srcSTR_2, ntrip_server_2);
    #endif
    reconnect_wifi(); // reconnect to WiFi if we lost the connection
    reset_counters_buffers(); // reset the counters and whipe the buffer
  } // end of if messages_in_buffer > 0

  read_serial(); // read all messages from the serial port to the buffer
}

/* 
 Contagem de saídas
 Versão: teste
 Autor: Victor Correia
 Alterado em: 25/02/2019
 */
 
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <EEPROMAnything.h>
#include <SD.h>

#define counterpin1 28
#define counterpin2 30
#define counterpin3 32
#define resetpin 40

int counter;

bool currentState = 0;
bool previousState = 0;
bool currentStateRes = 0;
bool previousStateRes = 0;


// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   50

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xF3 };
IPAddress ip(191, 188, 127, 50); 
EthernetServer server(8000);  
File webFile;   
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

void setup()
{
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);    
    Serial.begin(9600);      
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");   
    Ethernet.begin(mac, ip);  
    server.begin();          
    pinMode(counterpin1, INPUT); 
    digitalWrite(counterpin1, HIGH);
    pinMode(counterpin2, INPUT);
    digitalWrite(counterpin2, HIGH); 
    pinMode(counterpin3, INPUT); 
    digitalWrite(counterpin3, HIGH);
    pinMode(resetpin, INPUT); 
    digitalWrite(resetpin, HIGH);
    
    EEPROM_readAnything(102,counter);
       
}

void Read() {     
    if (digitalRead(counterpin1) == LOW ||digitalRead(counterpin2) == LOW ||digitalRead(counterpin3) == LOW) {  
        currentState = 1;
    }
    else {        
        currentState = 0;
    }
    if(currentState != previousState){
        if(currentState == 1){
            counter= counter + 1;
            EEPROM_writeAnything(102,counter);
            Serial.println(counter);
        }
    }
    previousState = currentState;    
}
void Reset() {
    if (digitalRead(resetpin) == LOW) { // check if the input is HIGH (button released)
        currentStateRes = 1;
    }
    else {        
        currentStateRes = 0;
    }
    if(currentStateRes != previousStateRes){
        if(currentStateRes == 1){
            counter = 0;
            EEPROM_writeAnything(102,counter);
            Serial.println(counter);
        }
    }
    previousStateRes = currentStateRes;    
}
// ficheiro XML com a contagem
void XML_response(EthernetClient cl){   
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    cl.print("<exitcounter>");
    cl.print(counter);
    cl.print("</exitcounter>");    
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length){
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

void Connection() {
  EthernetClient client = server.available();  // try to get client
  if (client) {  // got client?
      boolean currentLineIsBlank = true;
      while (client.connected()) {
          if (client.available()) {   // client data available to read
              char c = client.read(); // read 1 byte (character) from client
              // buffer first part of HTTP request in HTTP_req array (string)
              // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
              if (req_index < (REQ_BUF_SZ - 1)) {
                  HTTP_req[req_index] = c;          // save HTTP request character
                  req_index++;
              }
              // last line of client request is blank and ends with \n
              // respond to client only after last line received
              if (c == '\n' && currentLineIsBlank) {
                  // send a standard http response header
                  client.println("HTTP/1.1 200 OK");
                  // remainder of header follows below, depending on if
                  // web page or XML page is requested
                  // Ajax request - send XML file
                  if (strstr(HTTP_req, "ajax_inputs")) {
                      // send rest of HTTP header
                      client.println("Content-Type: text/xml");
                      client.println("Connection: keep-alive");
                      client.println("Cache-Control: no-cache");
                      client.println("Cache-Control: no-store");
                      client.println();
                      // send XML file containing input states
                      XML_response(client);
                  }
                  else if (strstr(HTTP_req, "COUNTER=0")) {
                      counter = 0;
                      EEPROM_writeAnything(102,counter);              
                  }                  
                  else {  // web page request
                      // send rest of HTTP header
                      client.println("Content-Type: text/html");
                      client.println("Connection: keep-alive");                      
                      client.println("Cache-Control: no-cache");
                      client.println("Cache-Control: no-store");
                      client.println();
                      // send web page
                      webFile = SD.open("index.htm");        // open web page file
                      if (webFile) {
                          while(webFile.available()) {
                              client.write(webFile.read()); // send web page to client
                          }
                          webFile.close();
                      }
                  }
                  // display received HTTP request on serial port
                  //Serial.print(HTTP_req);
                  // reset buffer index and all buffer elements to 0
                  req_index = 0;
                  StrClear(HTTP_req, REQ_BUF_SZ);
                  break;
              }
              // every line of text received from the client ends with \r\n
              if (c == '\n') {
                  // last character on line of received text
                  // starting new line with next character read
                  currentLineIsBlank = true;
              } 
              else if (c != '\r') {
                  // a text character was received from client
                  currentLineIsBlank = false;
              }
          } // end if (client.available())
      } // end while (client.connected())
      delay(1);      // give the web browser time to receive the data
      client.stop(); // close the connection
   } // end if (client)
}



void loop()
{
  Connection();
  Read();
  Reset();   
}

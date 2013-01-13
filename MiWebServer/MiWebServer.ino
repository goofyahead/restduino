/*
 Based on Web Server by Tom Igoe
 
 
 created 20 Oct 2011
 by Alejandro Vidal.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <String.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1, 39);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);
#define bufferMax 128
int bufferSize = 0;
char buffer[bufferMax];
char* primaryKey = "ABCD";
char **receivedCommands;

//pin rename
int DOOR = 2;

//functions def
void processGet(char** ptr);
void processPost(char** ptr);
void processCommands (char ** ptr);
char** parseCommands(char * string);

//debug mode
boolean debug = true;

void setup() {
  // Setup I/O pins
  pinMode(DOOR, OUTPUT); 
  // Open serial communications and wait for port to open:
  if (debug) {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  }
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  if (debug) {
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
  }
}

char** parseCommands(char * string) {
  char **commands;
  commands = (char **)malloc(sizeof(char *));
  char *p;
  p = strtok(string, " ");
  unsigned int i = 0;
  //Obtaing request method, GET or POST now in p
  commands[i] = p;
  i++;
  commands = (char **) realloc(commands, sizeof(char *) * i +1);
  //Save the command or commands /digital|analog/portNumber[/value]
  p = strtok(NULL, " ");

  char *command;
  command = strtok( p+1, "/");

  while (command != NULL) {
    commands[i]= command;
    i++;
    command = strtok(NULL, "/");
  }
  commands[i] = "\0";
  return commands;
}

void processCommands (EthernetClient client, char ** ptr){
  unsigned int i = 0;
  //process each command if POST set, if GET send response
  int x = strcmp(ptr[0],"GET");
  if (x == 0) {
    processGet(client, ptr);
  } 
  x = strcmp(ptr[0],"POST");
  if (x == 0) {
    processPost(client, ptr);
  }
  if (debug) {
    printPetition(ptr);
  }
}

void processGet (EthernetClient client, char** ptr) {
  int type = strcmp(ptr[1],"ANALOG");
  int pinNumber = atoi(ptr[2]);
  if (type == 0) {
    client.println(analogRead(pinNumber));
  } 
  else {
    client.println(digitalRead(pinNumber));
  }
}

void processPost (EthernetClient client, char** ptr) {
  int type = strcmp(ptr[1],"ANALOG");
  int pinNumber = atoi(ptr[2]);
  if (type == 0) {
    int value = atoi(ptr[3]);
    analogWrite(pinNumber,value);
  } 
  type = strcmp(ptr[1],"DIGITAL");
  if (type == 0) {
    int highOrLow = strcmp (ptr[3],"HIGH");
    if (highOrLow == 0) {
      digitalWrite(pinNumber,HIGH);
    }
    highOrLow = strcmp (ptr[3],"LOW");
    if (highOrLow == 0) {
      digitalWrite(pinNumber,LOW);  
    }
    highOrLow = strcmp(ptr[3], "TICK");
    if (highOrLow == 0) {
      digitalWrite(pinNumber, HIGH);
      delay(1500);
      digitalWrite(pinNumber,LOW);
    }
  }
  client.println("HTTP/1.1 200 OK");
}

void printPetition(char ** ptr) {
  Serial.print("PETICION:");
  Serial.print(ptr[0]);
  Serial.print(" IN MODE:");
  Serial.print(ptr[1]);
  Serial.print(" ON PIN:");
  Serial.print(ptr[2]);
  if (ptr[3] != "\0"){
    Serial.print(" WITH VALUE:");
    Serial.print(ptr[3]);
  }
  Serial.println("");
}

void sendResponse (EthernetClient client) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connnection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    int sensorReading = analogRead(analogChannel);
    client.print("analog input ");
    client.print(analogChannel);
    client.print(" is ");
    client.print(sensorReading);
    client.println("<br />");
  }
  client.println("</html>");
}

boolean parseKey(char * string) {
  char *keyPart = strtok(string, "?key=");
  keyPart = strtok(NULL, "?key=");
  char *key = strtok(keyPart, " ");
  if (strcmp(key, primaryKey) == 0) {
    return true;
  }
  else {
    return false;
  }
}

void updateMyIp() {
  EthernetClient client;
  delay(1000);
  if (client.connect("67.214.175.75", 80)) {
    if (debug) Serial.println("connected");
    // Make a HTTP request:
    client.println("GET /RemoteUpdate.sv?login=goofyahead&password=4F32GGJ21A&host=mylock.linkpc.net HTTP/1.0");
    client.println();
    
    while(client.connected()) {    
      // stay in this loop until the server closes the connection
       while(client.available())
       {
         // The server will not close the connection until it has sent the last packet
         // and this buffer is empty
         char read_char = client.read(); 
         if (debug) Serial.write(read_char);
       }
   }
   // close your end after the server closes its end
   client.stop();
  }
  else {
    // kf you didn't get a connection to the server:
    if (debug) Serial.println("connection failed");
  }
}

void loop() {
  // update ip if necesary
  updateMyIp();
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    //Serial.println("new client");
    bufferSize = 0;

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        if (bufferSize < bufferMax) {
          buffer[bufferSize++] = c;
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n') {
          boolean validKey = parseKey(buffer);
          if (!validKey) {
            client.println("HTTP/1.1 400");
            break;
          }
          receivedCommands = parseCommands(buffer);
          processCommands (client, receivedCommands);
          break;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    free(receivedCommands);
    buffer[0] = NULL;
    if (debug) {
      Serial.println("client disonnected");
      Serial.println("*************************************\n");
    }
  }
}



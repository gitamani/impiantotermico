/* ESP8266 Webserver 'MIT License (MIT) Copyright (c) 2016 by David Bird'
 * Creato da Giuseppe Tamanini
 * 10/12/2018
 * Licenza CC BY-NC-SA 3.0 IT
*/

#include <SoftwareSerial.h>

#define rxPin D5
#define txPin D6

// set up a new serial port
SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin);

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>     // https://github.com/tzapu/WiFiManager
#include "Gsender.h"

WiFiClient client;
ESP8266WebServer server(80); // Start server on port 80 (default for a web-browser, change to your requirements, e.g. 8080 if your Router uses port 80
                             // To access server from the outsid of a WiFi network e.g. ESP8266WebServer server(8266); and then add a rule on your Router that forwards a
                             // connection request to http://your_network_ip_address:8266 to port 8266 and view your ESP server from anywhere.
                             // Example http://g6ejd.uk.to:8266 will be directed to http://192.168.0.40:8266 or whatever IP address your router gives to this server

String webpage, Argument_Name, Clients_Response1, Clients_Response2;

String data; // data

// variabili da impostare
int tempPav[4]; // temperature ritorno impianto a pavimento
int BMTempPav[4]; // tolleranze temperatura (da 2 a 10) impianto a pavimento
int T_VZ; // tempo in secondi di apertura/chiusura valvola di zona dell'impianto a pavimento
int TT_VZ; // tempo in secondi per la totale apertura/chiusura valvola di zona
int T_att_ip; // tempo secondi di attesa prima della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
int T_att_st; // tempo in secondi di attesa prima della lettura della temperatura di ritorno S03 del solare termico
int TempV3vie; // temperatura uscita valvola a 3 vie (verrà calcolata dal programma in base alla temperatura esterna)
int T_V3vie; // tempo in secondi di apertura/chiusura valvola a 3 vie
int TT_V3vie; // tempo in secondi (dato di targa) di totale apertura/ciusura valvola a 3 vie
int T_att_V3vie; // tempo in secondi di attesa prima della rilettura della temperatura di uscita dalla valvola a 3 vie
int BMTempV3vie; // tolleranza temperatura uscita valvola 3 vie
int TempASMax; // temperatura massima acqua sanitaria
int BMTempAS; // tolleranza temperatura acqua sanitaria
int T_att_ir; // tempo in secondi di attesa prima della lettura della temperatura di uscita dall'impianto di riscaldamento
int T_att_as; // tempo in secondi di attesa prima della lettura della temperatura di uscita S9 dello scambiatore acqua calda sanitaria
int T_att_rc; // tempo in secondi di attesa prima della rilettura della temperatura della caldaia
int T_att_ca; // tempo in secondi di attesa prima della lettura della temperatura di ritorno S12 dal puffer alla caldaia
int Tempriltemp; // tempo in secondi di rilettura delle temperature rilevate dalle sonde

// variabili da visualizzare
int T[20]; // Temperature pavimento
boolean zona[4];
int paVZ[4]; // percentuale apertura valvole di zonba
boolean VZa[4]; // le valvola di zona aprono
boolean VZc[4]; // le valvola di zona chiudono
int Catt_VZ[4]; // ciclo attuale della valvola di zona
boolean erroreVZa[4]; // errore di apertura valvola di zona
boolean erroreVZc[4]; // errore di chiusura valcola di zona
int paV3vie; // percentuale apertura della valvola 3 vie
boolean Valvola3vie; // la valvola a 3 vie è in funzione?
boolean Valvola3viea; // la valvola a 3 vie apre
boolean Valvola3viec; // la valvola a 3 vie chiude
int Catt_V3vie; // ciclo attuale della valvola 3vie
boolean erroreV3viea; // errore di apertura valvola 3 vie
boolean erroreV3viec; // errore di chiusura valvola 3 vie
boolean erroreV3vie; // errore malfunzionamento pompa comandata da R7
boolean pompaST; // la pompa del solare termico è in funzione?
boolean erroreST; // errore pompa comandata da R4 solare termico
boolean pompaAS; // la pompa dello scambiatore dell'acqua sanitaria è in funzione?
boolean erroreAS; // errore pompa acqua calda sanitaria
boolean pompaCA; // la pompa uscita caldaia è in funzione
boolean erroreCA; // errore pompa comandata da R2 caldaia
boolean pompaIP; // pompa uscita valvola 3 vie verso impianto pavimento è in funzione
boolean erroreIP; // errore pompa comandata da R7 impianto riscaldamento
boolean MessageRC; // errore richiamo calore

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete
int n;
boolean complete; // ha completato di leggere i dati da Arduino Mega?

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  //WiFiManager intialisation. Once completed there is no need to repeat the process on the current board
  WiFiManager wifiManager;
  // New OOB ESP8266 has no Wi-Fi credentials so will connect and not need the next command to be uncommented and compiled in, a used one with incorrect credentials will
  // so restart the ESP8266 and connect your PC to the wireless access point called 'ESP8266_AP' or whatever you call it below in ""
  // wifiManager.resetSettings(); // Command to be included if needed, then connect to http://192.168.4.1/ and follow instructions to make the WiFi connection
  // Set a timeout until configuration is turned off, useful to retry or go to sleep in n-seconds
  wifiManager.setTimeout(180);
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "ESP8266_AP" and waits in a blocking loop for configuration
  if(!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println("failed to connect and timeout occurred");
    delay(3000);
    ESP.reset(); //reset and try again
    delay(5000);
  }
  // At this stage the WiFi manager will have successfully connected to a network, or if not will try again in 180-seconds
  //----------------------------------------------------------------------------------------------------------------------
  Serial.println("WiFi connected..");
  server.begin(); Serial.println("Webserver started..."); // Start the webserver
  Serial.print("Use this URL to connect: http://");// Print the IP address
  Serial.print(WiFi.localIP());Serial.println("/");
  // NOTE: You must use the IP address assigned to YOUR Board when printed/displayed here on your serial port
  // that's the address you must use, not the one I used !
  
  // Next define what the server should do when a client connects
  server.on("/", HandleClient); // The client connected with no arguments e.g. http:192.160.0.40/
  server.on("/result", ShowClientResponse);
  //sender();
}

void HandleClient() {
  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      if (server.argName(i) == "tempP2") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        tempPav[3] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "tempP1") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        tempPav[2] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "tempPT") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        tempPav[1] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "tempPint") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        tempPav[0] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "BMP2") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        BMTempPav[3] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "BMP1") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        BMTempPav[2] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "BMPT") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        BMTempPav[1] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "BMPint") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        BMTempPav[0] = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TVZ") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_VZ = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TTVZ") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        TT_VZ = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TAIP") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_att_ip = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TV3vie") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_V3vie = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TTV3vie") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        TT_V3vie = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TAV3vie") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_att_V3vie = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TAST") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_att_st = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "tempAS") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        TempASMax = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "BMPAS") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        BMTempAS = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TAAS") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_att_as = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TARC") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_att_rc = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TACA") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        T_att_ca = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
      if (server.argName(i) == "TRST") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Tempriltemp = server.arg(i).toInt();
        Clients_Response1 = server.arg(i);
      }
    }
  }
  creawebpage();
  server.send(200, "text/html", webpage); // Send a response to the client asking for input
  inviaDati();
}

void ShowClientResponse() {
  String webpage;
  webpage =  "<html>";
   webpage += "<head><title>Configurazione ESP8266</title>";
    webpage += "<style>";
     webpage += "body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}";
    webpage += "</style>";
   webpage += "</head>";
   webpage += "<body>";
    webpage += "<h1><br>ESP8266 Server - This was what the client sent</h1>";
    webpage += "<p>Name received was: " + Clients_Response1 + "</p>";
    webpage += "<p>Address received was: " + Clients_Response2 + "</p>";
   webpage += "</body>";
  webpage += "</html>";
  server.send(200, "text/html", webpage); // Send a response to the client asking for input
}

void loop() {
  if (complete) server.handleClient();
  caricaDati(); 
  delay(100);
}

void creawebpage() {
  webpage =  "<html>";
  webpage += "<head><title>Impianto Termico</title>";
  webpage += "<style>";
  webpage += "body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";
  webpage += "<script type='text/javascript'> var timeleft = 60; var downloadTimer = setInterval(function(){ timeleft--; document.getElementById('countdowntimer').textContent = timeleft; if(timeleft <= 0) clearInterval(downloadTimer); },1000); </script>";
  webpage += "<head> <meta http-equiv='refresh' content='60'> </head>";
  webpage += "<h1>GESTIONE IMPIANTO TERMICO</h1>";
  webpage += "<p>La pagina verr&agrave; aggiornata automaticamente fra <span id='countdowntimer'>10 </span> secondi</p>";
    String IPaddress = WiFi.localIP().toString();
    webpage += "<form action='http://"+IPaddress+"' method='POST'>";
     webpage += "<font size='4'>Temperatura impianto pavimento P.2: <input type='number' value='" + String(tempPav[0]) + "' min='20' max='50' name='tempP2'>";
     webpage += " Banda morta: <input type='number' value='" + String(BMTempPav[0]) + "' min='2' max='10' name='BMP2'>";
     webpage += " Step di apertura/chiusura valvole di zona (secondi): <input type='number' value='" + String(T_VZ) + "' min='1' max='30' name='TVZ'><br>";
     webpage += "Temperatura impianto pavimento P.1: <input type='number' value='" + String(tempPav[1]) + "' min='20' max='50' name='tempP1'>";
     webpage += " Banda morta: <input type='number' value='" + String(BMTempPav[1]) + "' min='2' max='10' name='BMP1'>";
     webpage += " Tempo totale di apertura/chiusura valvole di zona (secondi): <input type='number' value='" + String(TT_VZ) + "' min='30' max='120' name='TTVZ'><br>";
     webpage += "Temperatura impianto pavimento P.T: <input type='number' value='" + String(tempPav[2]) + "' min='20' max='50' name='tempPT  '>";
     webpage += " Banda morta: <input type='number' value='" + String(BMTempPav[2]) + "' min='2' max='10' name='BMPT'>";
     webpage += " Tempo di attesa prima della lettura della temperatura di uscita zona (secondi): <input type='number' value='" + String(T_att_ip) + "' min='30' max='3000' name='TAIP'><br>";
     webpage += "Temperatura impianto pavimento P.int: <input type='number' value='" + String(tempPav[3]) + "' min='20' max='50' name='tempPint'>";
     webpage += " Banda morta: <input type='number' value='" + String(BMTempPav[3]) + "' min='2' max='10' name='BMPint'><br>";
     webpage += "Banda morta valvola 3 vie: <input type='number' value='" + String(BMTempV3vie) + "' min='2' max='10' name='BMV3vie'><br>";
     webpage += "Step di apertura/chiusura valvola 3 vie (secondi): <input type='number' value='" + String(T_V3vie) + "' min='1' max='30' name='TV3vie'>";
     webpage += " Tempo totale di apertura/chiusura valvola 3 vie (secondi): <input type='number' value='" + String(TT_V3vie) + "' min='30' max='120' name='TTV3vie'><br>";
     webpage += "Tempo di attesa prima della rilettura della temperatura di ritorno impianti a pavimento (secondi): <input type='number' value='" + String(T_att_V3vie) + "' min='30' max='3000' name='TAV3vie'><br>";
     webpage += "Tempo di attesa prima della rilettura della temperatura di ritorno solare termico (secondi): <input type='number' value='" + String(T_att_st) + "' min='30' max='3000' name='TAST'><br>";
     webpage += "Temperatura massima acqua calda sanitaria: <input type='number' value='" + String(TempASMax) + "' min='40' max='55' name='tempAS'>";
     webpage += " Banda morta: <input type='number' value='" + String(BMTempAS) + "' min='2' max='10' name='BMPAS'><br>";
     webpage += "Tempo di attesa prima della lettura della temperatura di uscita dello scambiatore acqua calda sanitaria (secondi): <input type='number' value='" + String(T_att_as) + "' min='30' max='3000' name='TAAS'><br>";
     webpage += "Tempo di attesa prima della rilettura della temperatura della caldaia (secondi): <input type='number' value='" + String(T_att_rc) + "' min='30' max='3000' name='TARC'><br>";
     webpage += "Tempo di attesa prima della lettura della temperatura di ritorno dal puffer alla caldaia (secondi): <input type='number' value='" + String(T_att_ca) + "' min='30' max='3000' name='TACA'><br>";
     webpage += "Tempo di rilettura delle temperature rilevate dalle sonde (secondi): <input type='number' value='" + String(Tempriltemp) + "' min='30' max='3000' name='TRST'><br>";
     webpage += "<input type='submit' value='Salva impostazioni'>";
     webpage += "<h1>DATI IMPIANTO TERMICO</h1>";  
     webpage += "<b>IMPIANTO A PAVIMENTO:</b><br>";  
     String szona1, szona2, szona3, szona4, sVZa1, sVZa2, sVZa3, sVZa4, sVZc1, sVZc2, sVZc3, sVZc4;
     if (zona[0]) szona1 = "on "; else szona1 = "off";
     if (zona[1]) szona2 = "on "; else szona2 = "off";
     if (zona[2]) szona3 = "on "; else szona3 = "off";
     if (zona[3]) szona4 = "on "; else szona4 = "off";
     if (VZa[0]) sVZa1 = "apre "; else sVZa1 = "";
     if (VZa[1]) sVZa2 = "apre "; else sVZa2 = "";
     if (VZa[2]) sVZa3 = "apre "; else sVZa3 = "";
     if (VZa[3]) sVZa4 = "apre "; else sVZa4 = "";
     if (VZc[0]) sVZc1 = "chiude "; else sVZc1 = "";
     if (VZc[1]) sVZc2 = "chiude "; else sVZc2 = "";
     if (VZc[2]) sVZc3 = "chiude "; else sVZc3 = "";
     if (VZc[3]) sVZc4 = "chiude "; else sVZc4 = "";
     String seazona1, seazona2, seazona3, seazona4;
     if (erroreVZa[0]) seazona1 = "Errore apertura valvola "; else seazona1 = "";
     if (erroreVZa[1]) seazona2 = "Errore apertura valvola "; else seazona2 = "";
     if (erroreVZa[2]) seazona3 = "Errore apertura valvola "; else seazona3 = "";
     if (erroreVZa[3]) seazona4 = "Errore apertura valvola "; else seazona4 = "";
     String seczona1, seczona2, seczona3, seczona4;
     if (erroreVZc[0]) seczona1 = "Errore chiusura valvola "; else seczona1 = "";
     if (erroreVZc[1]) seczona2 = "Errore chiusura valvola "; else seczona2 = "";
     if (erroreVZc[2]) seczona3 = "Errore chiusura valvola "; else seczona3 = "";
     if (erroreVZc[3]) seczona4 = "Errore chiusura valvola "; else seczona4 = "";
     webpage += "Zona 4 (P.2): " + String(T[16 - 1]) + "&deg;C " + szona4 + sVZa4 + sVZc4 + paVZ[3] + "&#37; <font color='red'>" + seazona4 + seczona4 + "</font>- Zona 3 (P.1): " + String(T[15 - 1]) + "&deg;C " + szona3 + sVZa3 + sVZc3 + " " + paVZ[2] + "&#37; <font color='red'>" + seazona3 + seczona3 + "</font>- Zona 2 (P.T): " + String(T[14 - 1]) + "&deg;C " + szona2 + sVZa2 + sVZc2 + " " + paVZ[1] + "&#37; <font color='red'>" + seazona2 + seczona2 + "</font>- Zona 1 (P.int)1: " + String(T[13 - 1]) + "&deg;C " + szona1 + sVZa1 + sVZc1 + " " + paVZ[0] + "&#37; <font color='red'>" + seazona1 + seczona1 + "</font><br>";
     webpage += "<b>VALVOLA A 3 VIE:</b><br>";  
     webpage += "Temperatura esterna: " + String(T[1 - 1]) + "&deg;C - Temperatura calcolata alimentazione impianto a pavimento: " + String(TempV3vie) + "&deg;C</br>";
     String sValvola3vie, sValvola3viea, sValvola3viec, serroreV3viea, serroreV3viec,serroreV3vie;
     if (Valvola3vie) sValvola3vie = "on  "; else sValvola3vie = "off ";
     if (Valvola3viea) sValvola3viea = "apre "; else sValvola3viea = "";
     if (Valvola3viec) sValvola3viec = "chiude "; else sValvola3viec= "";
     if (erroreV3viea) serroreV3viea = "Errore apertura valvola 3 vie"; else serroreV3viea = "";
     if (erroreV3viec) serroreV3viec = "Errore chiusura valvola 3 vie"; else serroreV3viec = "";
     if (erroreV3vie) serroreV3vie = "Errore malfunzionamento pompa uscita valvola a 3 vie"; else serroreV3vie = "";
     webpage += sValvola3vie + sValvola3viea + sValvola3viec + paV3vie + "&#37; <font color='red'>" + serroreV3viea + serroreV3viec + "</font> - Temperatura puffer alto: " + String(T[4 - 1]) + "&deg;C - Temperatura uscita pompa: " + String(T[6 - 1]) + "&deg;C - Temperatura ritorno da impianti a pavimento: " + String(T[7 - 1]) + "&deg; <font color='red'>" + serroreV3vie +"</font><br>";
     webpage += "<b>SOLARE TERMICO:</b><br>";  
     String spompaST, seST;
     if (pompaST) spompaST = "on "; else spompaST = "off";
     if (erroreST) seST = "Errore malfunzionamento pompa solare termico"; else seST = "";
     webpage += "Pompa R4: " + spompaST + " - Temperatura puffer basso: " + String(T[5 - 1]) + "&deg;C - Temperatura pannelli: " + String(T[2 - 1]) + "&deg;C - Temperatura entrata puffer: " + String(T[3 - 1]) + "&deg; <font color='red'>" + seST +"</font><br>";
     webpage += "<b>ACQUA CALDA SANITARIA:</b><br>";  
     String spompaAS, seAS;
     if (pompaAS) spompaAS = "on "; else spompaAS = "off";
     if (erroreAS) seAS = "Errore malfunzionamento pompa acqua calda sanitaria"; else seAS = "";
     webpage += "Pompa R3: " + spompaAS + " - Temperatura puffer alto: " + String(T[4 - 1]) + "&deg;C - Temperatura ingresso scambiatore: " + String(T[8 - 1]) + "&deg;C - Temperatura uscita scambiatore: " + String(T[9 - 1]) + "&deg; <font color='red'>" + seAS +"</font><br>";
     webpage += "<b>CALDAIA:</b><br>";  
     String spompaCA, seCA;
     if (pompaCA) spompaCA = "on "; else spompaCA = "off";
     if (erroreCA) seCA = "Errore malfunzionamento pompa caldaia"; else seCA = "";
     webpage += "Pompa R2: " + spompaCA + " - Temperatura caldaia: " + String(T[10 - 1]) + "&deg;C - Temperatura uscita pompa: " + String(T[11 - 1]) + "&deg;C - Temperatura ritorno da puffer alto: " + String(T[12 - 1]) + "&deg; <font color='red'>" + seCA +"</font><br>";
     //webpage += "<meta http-equiv='refresh' content='1'>";
    webpage += "</form>";
   webpage += "</body>";
  webpage += "</html>";
}

void serialEvent() {
  while (mySerial.available()) {
    // get the new byte:
    char inChar = (char)mySerial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      inputString = inputString.substring(0, inputString.length() - 1);
      stringComplete = true;
    }
  }
}

void caricaDati() {
  serialEvent();
  if (inputString != "") {
    Serial.print("inputString:");
    Serial.println(inputString);
    String sm = inputString.substring(0,2);
    int m = sm.toInt();
    Serial.print("n:");
    Serial.println(m);
    inputString = inputString.substring(2, inputString.length());
    Serial.println(inputString);
    switch (m + 1) {
      case 1:
        data = inputString;
        break;
      case 2:
        tempPav[0] = inputString.toInt();
        break;
      case 3:
        tempPav[1] = inputString.toInt();
        break;
      case 4:
        tempPav[2] = inputString.toInt();
        break;
      case 5:
        tempPav[3] = inputString.toInt();
        break;
      case 6:
        BMTempPav[0] = inputString.toInt();
        break;
      case 7:
        BMTempPav[1] = inputString.toInt();
        break;
      case 8:
        BMTempPav[2] = inputString.toInt();
        break;
      case 9:
        BMTempPav[3] = inputString.toInt();
        break;
      case 10:
        T_VZ = inputString.toInt() / 1000;
        break;
      case 11:
        TT_VZ = inputString.toInt() / 1000;
        break;
      case 12:
        T_att_ip = inputString.toInt() / 1000;
        break;
      case 13:
        T_att_st  = inputString.toInt() / 1000;
        break;
      case 14:
        T_V3vie = inputString.toInt() / 1000;
        break;
      case 15:
        TT_V3vie = inputString.toInt() / 1000;
        break;
      case 16:
        T_att_V3vie = inputString.toInt() /1000;
        break;
      case 17:
        BMTempV3vie = inputString.toInt();
        break;
      case 18:
        T_att_ir = inputString.toInt();
        break;
      case 19:
        TempASMax = inputString.toInt();
        break;
      case 20:
        BMTempAS = inputString.toInt();
        break;
      case 21:
        T_att_rc = inputString.toInt() / 1000;
        break;
      case 22:
        T_att_as = inputString.toInt() / 1000;
        break;
      case 23:
        T_att_ca = inputString.toInt() / 1000;
        break;
      case 24:
        Tempriltemp = inputString.toInt() / 1000;
        break;
      case 25:
        zona[0] = inputString.toInt();
        break;
      case 26:
        zona[1] = inputString.toInt();
        break;
      case 27:
        zona[2] = inputString.toInt();
        break;
      case 28:
        zona[3] = inputString.toInt();
        break;
      case 29:
        T[0] = inputString.toInt();
        break;
      case 30:
        T[1] = inputString.toInt();
        break;
      case 31:
        T[2] = inputString.toInt();
        break;
      case 32:
        T[3] = inputString.toInt();
        break;
      case 33:
        T[4] = inputString.toInt();
        break;
      case 34:
        T[5] = inputString.toInt();
        break;
      case 35:
        T[6] = inputString.toInt();
        break;
      case 36:
        T[7] = inputString.toInt();
        break;
      case 37:
        T[8] = inputString.toInt();
        break;
      case 38:
        T[9] = inputString.toInt();
        break;
      case 39:
        T[10] = inputString.toInt();
        break;
      case 40:
        T[11] = inputString.toInt();
        break;
      case 41:
        T[12] = inputString.toInt();
        break;
      case 42:
        T[13] = inputString.toInt();
        break;
      case 43:
        T[14] = inputString.toInt();
        break;
      case 44:
        T[15] = inputString.toInt();
        break;
      case 45:
        T[16] = inputString.toInt();
        break;
      case 46:
        T[17] = inputString.toInt();
        break;
      case 47:
        T[18] = inputString.toInt();
        break;
      case 48:
        T[19] = inputString.toInt();
        break;
      case 49:
        VZa[0] = inputString.toInt();
        break;
      case 50:
        VZa[1] = inputString.toInt();
        break;
      case 51:
        VZa[2] = inputString.toInt();
        break;
      case 52:
        VZa[3] = inputString.toInt();
        break;
      case 53:
        VZc[0] = inputString.toInt();
        break;
      case 54:
        VZc[1] = inputString.toInt();
        break;
      case 55:
        VZc[2] = inputString.toInt();
        break;
      case 56:
        VZc[3] = inputString.toInt();
        break;
      case 57:
        Catt_VZ[0] = inputString.toInt();
        break;
      case 58:
        Catt_VZ[1] = inputString.toInt();
        break;
      case 59:
        Catt_VZ[2] = inputString.toInt();
        break;
      case 60:
        Catt_VZ[3] = inputString.toInt();
        break;
      case 61:
        erroreVZa[0] = inputString.toInt();
        break;
      case 62:
        erroreVZa[1] = inputString.toInt();
        break;
      case 63:
        erroreVZa[2] = inputString.toInt();
        break;
      case 64:
        erroreVZa[3] = inputString.toInt();
        break;
      case 65:
        erroreVZc[0] = inputString.toInt();
        break;
      case 66:
        erroreVZc[1] = inputString.toInt();
        break;
      case 67:
        erroreVZc[2] = inputString.toInt();
        break;
      case 68:
        erroreVZc[3] = inputString.toInt();
        break;
      case 69:
        pompaST = inputString.toInt();
        break;
      case 70:
        erroreST = inputString.toInt();
        break;
      case 71:
        TempV3vie = inputString.toInt();
        break;
      case 72:
        Valvola3viea = inputString.toInt();
        break;
      case 73:
        Valvola3viec = inputString.toInt();
        break;
      case 74:
        Catt_V3vie = inputString.toInt();
        break;
      case 75:
        erroreV3viea = inputString.toInt();
        break;
      case 76:
        erroreV3vie = inputString.toInt();
        break;
      case 77:
        pompaAS = inputString.toInt();
        break;
      case 78:
        erroreAS = inputString.toInt();
        break;
      case 79:
        pompaCA = inputString.toInt();
        break;
      case 80:
        erroreCA = inputString.toInt();
        break;
      case 81:
        pompaIP = inputString.toInt();
        break;
      case 82:
        erroreIP = inputString.toInt();
        break;
      case 83:
        MessageRC = inputString.toInt();
        complete = true;
        break;
    }
    inputString = "";
    stringComplete = false;
  }
}

void inviaDati() {
  for (int m = 0; m < 22; m++) {
    Serial.println(m);
    char buffer[2];
    sprintf(buffer, "%02d", m);
    mySerial.print(buffer);
    switch (m + 1) {
      case 1:
        mySerial.println(tempPav[0]);
        break;
      case 2:
        mySerial.println(tempPav[1]);
        break;
      case 3:
        mySerial.println(tempPav[2]);
        break;
      case 4:
        mySerial.println(tempPav[3]);
        break;
      case 5:
        mySerial.println(BMTempPav[0]);
        break;
      case 6:
        mySerial.println(BMTempPav[1]);
        break;
      case 7:
        mySerial.println(BMTempPav[2]);
        break;
      case 8:
        mySerial.println(BMTempPav[3]);
        break;
      case 9:
        mySerial.println(T_VZ);
        break;
      case 10:
        mySerial.println(TT_VZ);
        break;
      case 11:
        mySerial.println(T_att_ip);
        break;
      case 12:
        mySerial.println(T_att_st);
        break;
      case 13:
        mySerial.println(T_V3vie);
        break;
      case 14:
        mySerial.println(TT_V3vie);
        break;
      case 15:
        mySerial.println(T_att_V3vie);
        break;
      case 16:
        mySerial.println(BMTempV3vie);
        break;
      case 17:
        mySerial.println(TempASMax);
        break;
      case 18:
        mySerial.println(BMTempAS);
        break;
      case 19:
        mySerial.println(T_att_as);
        break;
      case 20:
        mySerial.println(T_att_rc);
        break;
      case 21:
        mySerial.println(T_att_ca);
        break;
      case 22:
        mySerial.println(Tempriltemp);
        break;
    }
    delay(100);
  }
}

void sender() {
  Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
  String subject = "Inviato da ESP8266";
  if(gsender->Subject(subject)->Send("bepitama@gmail.com", "Messaggio di prova")) {
    Serial.println("Message send.");
  } else {
    Serial.print("Error sending message: ");
    Serial.println(gsender->getError());
  }
}

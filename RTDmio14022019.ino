/*Creato da Giuseppe Tamanini
 * 09/02/2019
 * Licenza CC BY-NC-SA 3.0 IT
*/

#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <Wire.h>
// Librerie display oled 128x64 SSD1306 connesso via I2C
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// Indirizzo MAC della Ethernet Shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// L'indirizzo IP da assegnare alla Ethernet Shield (dipende dalla vostra rete locale)
IPAddress ip(192, 168, 1, 177);

// Inizializza la libreria Ethernet per il server usando la porta 80
EthernetServer server(80);

// Libreria DS1307 RTC connesso via I2C
#include "RTClib.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiWire oled;
RTC_DS1307 rtc;

String bufferArgs;
int argChars = 0;
String arg;
String value;

int sensorPin[5] = {A0, A1, A2, A3, A4}; // pin analogico uscita RTD x 4 ingressi PT1000
int selPinA[5] = {38, 40, 42, 44, 46};   // pin A di commutazione dei multiplexer
int selPinB[5] = {39, 41, 43, 45, 47};   // pin B di commutazione dei multiplexer
// pin 38 e 39 prima scheda RTD per sonde S01 - S04
// pin 40 e 41 seconda scheda RTD per sonde S05 - S08
// pin 42 e 43 terza scheda RTD per sonde S09 - S12
// pin 44 e 45 quarta scheda RTD per sonde S13 - S16
// pin 46 e 47 quinta scheda RTD per sonda S17 - S20
int inputPinC[4] = {2, 3, 5, 6}; // pin digitali in ingresso che leggono se le valvole di zona sono chiuse
// pin 2 chiusura valvola P.int
// pin 3 chiusura valvola P.T
// pin 4 chiusura valvola P.1
// pin 5 chiusura valvola P.2
int inputPinA[4] = {8, 9, A9, A10}; // pin digitali in ingresso che leggono se le valvole di zona sono aperte
// pin 6 apertura valvola P.int
// pin 7 apertura valvola P.T
// pin 8 apertura valvola P.1
// pin 9 apertura valvola P.2
int inputPinV3vieC = 7; // pin 10 chiusura valvola 3 vie
int inputPinV3vieA = A11; // pin 11 apertura valvola 3 vie
boolean dataPinAB[2] [4] = {{1, 0, 0, 1}, {0, 1, 0, 1}}; // dati per la commutazione dei multiplexer
// 10 seleziona l'ingresso 1 della scheda RTD
// 01 seleziona l'ingresso 2 della scheda RTD
// 00 seleziona l'ingresso 3 della scheda RTD
// 11 seleziona l'ingresso 4 della scheda RTD
int relePin[16] = {22, 24, 26, 28, 30, 32, 34, 36, 23, 25, 27, 29, 31, 33, 35, 37}; // pin dove sono collegati i relè
// pin 22 relè 02 Esterno caldaia
// pin 24 relè 03 Acqua calda sanitaria
// pin 26 relè 04 Solare termico
// pin 28 relè 05 chiude valvola tre vie
// pin 30 relè 06 apre valvola tre vie
// pin 32 relè 07 pompa mandata impianto a pavimento
// pin 34 relè 08 non usato
// pin 36 relè 09 chiude valvola prima zona impianto a pavimento (P.int)
// pin 23 relè 10 apre valvola prima zona impianto a pavimento (P.int)
// pin 25 relè 11 chiude valvola seconda zona impianto a pavimento (P.T)
// pin 27 relè 12 apre valvola seconda zona impianto a pavimento (P.T)
// pin 29 relè 13 chiude valvola prima zona impianto a pavimento (P.1)
// pin 31 relè 14 apre valvola prima zona impianto a pavimento (P.1)
// pin 33 relè 15 chiude valvola prima zona impianto a pavimento (P.2)
// pin 35 relè 16 apre valvola prima zona impianto a pavimento (P.2)
// pin 37 relè 17 non usato

int sensorValue = 0; // memorizza il valore letto dalle sonde
int Sonda[20]; // matrice sonde temperatura
// Sonda 01 temperatura esterna
// Sonda 02 temperatura acqua pannelli solare termico
// Sonda 03 temperatura acqua pannelli solare termico esterna puffer (malfunzionamento solare termico)
// Sonda 04 temperatura puffer
// Sonda 05 temperatura acqua pannelli solare termico interna puffer
// Sonda 06 temperatura uscita valvola tre vie
// Sonda 07 temperatura ritorno globale impianto a pavimento
// Sonda 08 temperatura entrata scambiatore calore acqua calda sanitaria
// Sonda 09 temperatura uscita scambiatore calore acqua calda sanitaria
// Sonda 10 temperatura caldaia
// Sonda 11 temperatura uscita caldaia
// Sonda 12 temperatura ritorno caldaia
// Sonda 13 temperatura ritorno prima zona impianto a pavimento (P.int)
// Sonda 14 temperatura ritorno seconda zona impianto a pavimento (P.T)
// Sonda 15 temperatura ritorno terza zona impianto a pavimento (P.1)
// Sonda 16 temperatura ritorno quarta zona impianto a pavimento (P.2)
// Sonda 17 temperatura ambiente (P.int)
// Sonda 18 temperatura ambiente (P.T)
// Sonda 19 temperatura ambiente (P.1)
// Sonda 20 temperatura ambiente (P.2)

boolean Zona[4]; // la zona è attiva? (se la zona non è attiva la volvola di zona viene chiusa)
boolean Vzona[4]; // la valvola di zona è in funzione?
boolean VzonaA[4]; // la valvola di zona è in fase di apertura;
boolean VzonaC[4]; // la valvola di zona è in fase di chiusura;
boolean Pinatt[4]; // è in attesa della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
boolean Zinoff[4]; // è in chiusura della valvola di zona per la disattivazione della zona
int Catt_VZ[4] = {0, 0, 0, 0}; // ciclo attuale della valvola di zona
int paVZ[4]; // percentuale apertura valvole di zona
boolean VZoff[4]; // la valvola di zona è ferma
boolean erroreVZ_A[4]; // errore di apertura valvola di zona
boolean erroreVZ_C[4]; // errore di chiusura valcola di zona
boolean TVZoff; // tutte le valvole di zona sono chiuse?
boolean pompaST; // la pompa del solare termico è in funzione?
boolean erroreST; // errore pompa comandata da R4 solare termico
boolean Valvola3vieA; // la valvola a 3 vie è in fase di apertura?
boolean Valvola3vieC; // la valvola a 3 vie è in fase di chiusura?
boolean Valv3vieinattA; // in apertura è in attesa della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
boolean Valv3vieinattC; // in chiusura è in attesa della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
int Catt_V3vie = 10; // ciclo attuale della valvola 3vie
int paV3vie; // percentuale apertura della valvola 3 vie
boolean V3vieoff; // il finecorsa della valvola 3 vie in chiusura è on
boolean V3vieon; // il finecorsa  della valvola 3 vie in apertura è on
boolean erroreV3vieA; // errore di apertura valvola 3 vie
boolean erroreV3vieC; // errore di chiusura valvola 3 vie
boolean pompaAS; // la pompa dello scambiatore dell'acqua sanitaria è in funzione?
boolean erroreAS; // errore pompa acqua calda sanitaria
boolean pompaCA; // la pompa uscita caldaia è in funzione
boolean erroreCA; // errore pompa comandata da R2 caldaia
boolean pompaIP; // pompa in uscita dalla valvola a 3vie verso l'impianto a pavimento?
boolean erroreIP; // errore malfunzionamento valvola 3 vie o pompa comandata da R7
boolean lettemp; // ha già letto le temperature?
boolean MessageRC; // Invia una email di Messaggio Richiesta Calore

int TempPav[4]; // temperature ritorno impianto a pavimento
int BMTempPav[4]; // tolleranze temperatura impianto a pavimento
long T_VZ; // tempo in millisecondi di apertura/chiusura valvola di zona dell'impianto a pavimento
long TT_VZ; // tempo in millesecondi (dato di targa) per la totale apertura/chiusura valvola di zona
long T_att_ip; // tempo (da 30000 a 600000) millisecondi di attesa prima della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
long T_att_st; // tempo 300000 millisecondi di attesa prima della lettura della temperatura di ritorno S03 del solare termico
int TempV3vie; // temperatura uscita valvola a 3 vie (verrà calcolata dal programma in base alla temperatura esterna)
long T_V3vie; // tempo in millisecondi (5 secondi) di apertura/chiusura valvola a 3 vie
long TT_V3vie; // tempo in millisecondi di totale apertura/ciusura valvola a 3 vie
long T_att_V3vie; // tempo in millisecondi di attesa prima della rilettura della temperatura di uscita dalla valvola a 3 vie
int BMTempV3vie; // tolleranza Temperatura uscita valvola 3 vie
long T_att_ir; // tempo in millisecondi di attesa prima della lettura della temperatura di ritorno dall'impianto di riscaldamento
long T_att_rc; // tempo in millisecondi di attesa prima della lettura della temperatura di ritorno della caldaia
long T_att_as; // tempo in millisecondi di attesa prima della lettura della temperatura di uscita S9 dello scambiatore acqua calda sanitaria
long T_att_ca; // tempo in millisecondi di attesa prima della lettura della temperatura di ritorno S12 dal puffer alla caldaia
int BMTempAS; // tolleranza Temperatura acqua sanitaria
long Tempriltemp; // tempo in millisecondi di rilettura delle temperature rilevate dalle sonde
int nc_VZ; // numero dei cicli di apertura/chiusura delle valvole di zona
int nc_V3vie; // numero dei cicli di apertura/chiusura delle valvola 3 vie
int TempASMax; // temperatura massima acqua calda sanitaria

const int TempPavMin = 6; // allarme di TempPav minima (soglia antigelo)
const int TempPavMax = 50; // allarme di TempPav massima

const int TempPaMin = 25; // temperatura minima Puffer alto

int anno; // variabili dell'orologio
int mese;
int giorno;
int ora;
int minuti;
int oldminuti;
int giornosettimana;
String sgiornosettimana[7] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};

static unsigned long myTime; // tempo di apertura/chiusura valvole pavimento
static unsigned long myTime1; // tempo di attesa rilettura valvole pavimento
static unsigned long myTime2; // tempo di attesa lettura S3 del solare termico
static unsigned long myTime3; // tempo di apertura/chiusura valvola 3 vie
static unsigned long myTime4; // tempo di attesa rilettura S6 uscita valvola a 3 vieZona[i]
static unsigned long myTime5; // tempo di attesa lettura S13 ritorno caldaia
static unsigned long myTime6; // tempo di chiusura valvole pavimento disattivazione zone
static unsigned long myTime7; // tempo di attesa della rilettura delle temperature
static unsigned long myTime8; // tempo di attesa della temperatura di uscita S9 dello scambiatore acqua calda sanitaria
static unsigned long myTime9; // tempo di attesa della temperatura di ritorno S12 dal puffer alla caldaia
static unsigned long myTime0; // tempo di attesa della temperatura di ritorno S7 dagli impianti a pavimento
static unsigned long myTime10; // tempo di attesa del rivio dei dati

File myFile; // file aperto in SD

void setup() {
  Serial.begin(115200);
  analogReference(AR_DEFAULT);
  delay(3000);
  Serial.println("Connesso");
  
  Wire.begin();
  Wire.setClock(400000L);

  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0
  oled.setFont(Callibri10);
  oled.clear();
  
  if (! rtc.begin()) {
    Serial.println("RTC non trovato");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC non è avviato");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.print("Inizializza SD card...");

  if (!SD.begin(4)) {
    Serial.println("Inizializzazione fallita!");
    while (1);
  }
  Serial.println("Inizializzazione effettuata");
  if (SD.exists("dati.txt")) {
    Serial.println("dati.txt exists.");
  } 
  myFile = SD.open("dati.txt"); // apre il file
  if (myFile) {  // se il file esiste
    while (myFile.available()) { // legge i dati fino alla fine del file
      String riga;
      Zona[0]= myFile.readStringUntil('\n').toInt();
      Zona[1]= myFile.readStringUntil('\n').toInt();
      Zona[2]= myFile.readStringUntil('\n').toInt();
      Zona[3]= myFile.readStringUntil('\n').toInt();
      TempPav[0] = myFile.readStringUntil('\n').toInt();
      TempPav[1] = myFile.readStringUntil('\n').toInt();
      TempPav[2] = myFile.readStringUntil('\n').toInt();
      TempPav[3] = myFile.readStringUntil('\n').toInt();
      BMTempPav[0] = myFile.readStringUntil('\n').toInt();
      BMTempPav[1] = myFile.readStringUntil('\n').toInt();
      BMTempPav[2] = myFile.readStringUntil('\n').toInt();
      BMTempPav[3] = myFile.readStringUntil('\n').toInt();
      T_VZ = myFile.readStringUntil('\n').toInt()*1000;
      TT_VZ = myFile.readStringUntil('\n').toInt()*1000;
      T_att_ip = myFile.readStringUntil('\n').toInt()*1000;
      T_att_st  = myFile.readStringUntil('\n').toInt()*1000;
      T_V3vie = myFile.readStringUntil('\n').toInt()*1000;
      TT_V3vie = myFile.readStringUntil('\n').toInt()*1000;
      T_att_V3vie = myFile.readStringUntil('\n').toInt()*1000;
      BMTempV3vie = myFile.readStringUntil('\n').toInt();
      T_att_ir = myFile.readStringUntil('\n').toInt()*1000;
      TempASMax = myFile.readStringUntil('\n').toInt();
      BMTempAS = myFile.readStringUntil('\n').toInt();
      T_att_rc = myFile.readStringUntil('\n').toInt()*1000;
      T_att_as = myFile.readStringUntil('\n').toInt()*1000;
      T_att_ca = myFile.readStringUntil('\n').toInt()*1000;
      Tempriltemp = myFile.readStringUntil('\n').toInt()*1000;
    }
  }
  myFile.close();
  // dichiara i vari inputPin e selPin in INPUT/OUTPUT
  for (int i = 0; i < 4; i++) {
    pinMode(inputPinC[i], INPUT);
    pinMode(inputPinA[i], INPUT);
  }
  for (int i = 0; i < 5; i++) {
    pinMode(selPinA[i], OUTPUT);
    pinMode(selPinB[i], OUTPUT);    
  }
  // dichiara i relePin come OUTPUT e li setta come HIGH (contatti chiusi)
  for (int i = 0; i < 16; i++) {
    pinMode(relePin[i], OUTPUT);
    digitalWrite(relePin[i], HIGH);
  }
  // setta le Zone come attive e le valvole di zona spente
  for (int i = 0; i < 4 ; i++) {
    Zona[i] = true;
    VZoff[i] = false;
  }
  chiude_V();
  nc_VZ = TT_VZ / T_VZ; // calcola il numero dei cicli che servono per completare l'apertura/chiusura delle valvole di zona
  if (nc_VZ * 5000 < TT_VZ) nc_VZ = nc_VZ + 1; // se il risultato non è intero arrotonda il valore a quello successivo
  nc_V3vie = TT_V3vie / T_V3vie; // calcola il numero dei cicli che servono per completare l'apertura/chiusura della valvola 3 vie
  if (nc_V3vie * 5000 < TT_V3vie) nc_V3vie = nc_V3vie + 1; // se il risultato non è intero arrotonda il valore a quello successivo
  
  // avvia la connessione Ethernet del server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Il Server è attivo con IP: ");
  Serial.println(Ethernet.localIP());
  delay(1000);
}

void loop() {
  DateTime now = rtc.now(); // legge i dati dal modulo RTC
  anno = now.year(); // memorizza i dati di data e ora nelle variabili
  mese = now.month();
  giorno = now.day();
  ora = now.hour();
  minuti = now.minute();
  oled.setCursor(0, 6);
  giornosettimana = now.dayOfTheWeek();
  oled.print(sgiornosettimana[giornosettimana]);
  oled.print(" ");
  oled.print(String(giorno));
  oled.print("/");
  oled.print(String(mese));
  oled.print("/");
  oled.print(String(anno));
  oled.print(" ");
  oled.print(String(ora));
  oled.print(":");
  oled.print(String(minuti));
  leggitemperature(); // esegue la procedura (void) che legge le temperature
  for (int i = 0; i < 4; i++) { // esegue la procedura (void) che controlla l'impianti a pavimento delle 4 zone (da 0 a 3)
    pavimento(i);
  }
  if (TVZoff == false) { // se almeno una delle zone è attiva
    for (int i = 0; i < 4; i++) { 
      if (Pinatt[i] == true && pompaIP == false) {
        digitalWrite(relePin[7 - 1], LOW);; // accende il relè 07 che comanda la pompa uscita valvola 3 vie
        pompaIP = true;
        Serial.println("R7acceso");
        break;
      }
    }
  }
  TVZoff = true; // pone la variabile TVZoff a vero
  for (int i = 0; i < 4; i++) { // controlla se le valvole di zona sono chiuse 
    VZoff[i] = digitalRead(inputPinC[i]); // legge il pin digitale che rileva se la valvola di zona è chiusa
    if (VZoff[i] == false) { // nel caso una qualsiasi non lo sia
      TVZoff = false; // pone la variabile TVZoff a falso
      if (Pinatt[i]) {
        digitalWrite(relePin[i + 8], HIGH); // spegne il relè che chiude la valvola di zona
        digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona
      }
      break; // esce dal ciclo for
    }
  }
  if (TVZoff) { // se tutte le valvole di zona sono chiuse
    if (pompaIP) {
      digitalWrite(relePin[7 - 1], HIGH); // spegne il relè 07 che comanda la pompa uscita valvola 3 vie
      Serial.println("R7spento");
      pompaIP = false;
    }
  }
  //solaretermico();
  valvola3vie();
  //acquasanitaria();
  //caldaia();
  createWebPage();
  delay(250);
}

void pavimento(int i) {
  Serial.print("S");
  Serial.print(i + 13);
  Serial.print(":");
  Serial.println(Sonda[i + 12]);
  oled.setCursor(0 + i * 32, 0);
  oled.print("Z");
  oled.print(i);
  oled.print(":");
  oled.print(Sonda[i + 12]);
  oled.print("`");
  oled.setCursor(0 + i * 32, 1);
  if (Zona[i]) oled.print("on  ");
  if (Zona[i] == false) oled.print("off ");
  oled.print((int)((float)Catt_VZ[i] / (float)nc_VZ * 100));
  VZoff[i] = digitalRead(inputPinA[i]); // legge il pin digitale che rileva se la valvola è aperta
  if (Zona[i] && VZoff[i] == false && erroreVZ_A[i] == false && Sonda[i + 12] < TempPav[i] - BMTempPav[i] / 2) {
    // se la zona è attiva, non c'è errore di apertura e la temperatura di ritorno della zona è minore di quella impostata meno banda morta
    VzonaA[i] = true; // la valvola di zona è in fase di chiusura
    if (Vzona[i] == false && Pinatt[i] == false) {
      myTime = millis(); // azzera il tempo di apertura della valvola
      Serial.print("VZ");
      Serial.print(i);
      Serial.println("apre");
      Serial.println(Catt_VZ[i]);
      digitalWrite(relePin[i + 10], LOW); // accende il relè che apre la valvola di zona
      Vzona[i] = true; // a valvola della zona è in funzione
      Catt_VZ[i] = Catt_VZ[i] + 1; // aumenta il numero dei cicli di apertura
      if (Catt_VZ[i] > nc_VZ) {
        Catt_VZ[i] = nc_VZ;
        erroreVZ_A[i] = true; // se ha superato i cicli previsti attiva l'errore
        Serial.print("Errore di apertura valvola di zona: ");
        Serial.println(i);
      }
    } else if (Pinatt[i] == false) {
      if (millis() - myTime > T_VZ) { // è passato il tempo di apertura della valvola
        Serial.print("VZ");
        Serial.print(i);
        Serial.println("spenta");
        digitalWrite(relePin[i + 10], HIGH); // spegne il relè che apre la valvola di zona
        Pinatt[i] = true; // è in attesa della rilettura della temperatura di ritorno della zona
        Vzona[i] = false;  // la valvola della zona è ferma
        myTime1 = millis();  // azzera il tempo di attesa della rilettura della temperatura di ritorno della zona
      }
    } else {
      if (millis() - myTime1 > T_att_ip) { //  è passato il tempo di rilettura della temperatura di ritorno della zona
        Pinatt[i] = false; // non è più in attesa della rilettura della temperatura di ritorno della zona
      }
    }
  }
  VZoff[i] = digitalRead(inputPinC[i]); // legge il pin digitale che rileva se la valvola è chiusa
  if (Zona[i] && VZoff[i] == false && erroreVZ_C[i] == false && Sonda[i + 12] > TempPav[i] + BMTempPav[i] / 2) {
    // se la zona è attiva, non c'è errore di chiusura e la temperatura di ritorno della zona è maggiore di quella impostata più banda morta
    VzonaC[i] = true; // la valvola di zona è in fase di chiusura;
    if (Vzona[i] == false && Pinatt[i] == false) {
      Serial.print("VZ");
      Serial.print(i);
      Serial.println("chiude");
      Serial.println(Catt_VZ[i]);
      myTime = millis(); // azzera il tempo di chiusura della valvola
      digitalWrite(relePin[i + 9], LOW); // accende il relè che chiude la valvola di zona
      Vzona[i] = true; // la valvola della zona è in funzione
      Catt_VZ[i] = Catt_VZ[i] - 1; // decrementa il numero dei cicli di chiusura
      if (Catt_VZ[i] <= 0) {
        Catt_VZ[i] = 0;
        erroreVZ_C[i] = true; // se ha superato i cicli previsti attiva l'errore
        Serial.print("Errore di chiusura valvola di zona: ");
        Serial.println(i);
        digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona
      }
    } else if (Pinatt[i] == false) {
      if (millis() - myTime > T_VZ) { // è passato il tempo di chiusura della valvola
      Serial.print("VZ");
      Serial.print(i);
      Serial.println("spenta");
        digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona
        Pinatt[i] = true; // è in attesa della rilettura della temperatura di ritorno della zona
        Vzona[i] = false;  // la valvola della zona è ferma
        myTime1 = millis(); // azzera il tempo di attesa della rilettura della temperatura di ritorno della zona
      }
    } else {
      if (millis() - myTime1 > T_att_ip) { //  è passato il tempo di rilettura della temperatura di ritorno della zona
        Pinatt[i] = false; // non è più in attesa della rilettura della temperatura di ritorno della zona
      }
    }
  }
  if (Zona[i] == false && VZoff[i] == false && erroreVZ_C[i] == false) {
    VzonaA[i] = false;
    VzonaC[i] = false;
    // se la zona è attiva, non è in chiusura e non è in errore di chiusura disattiva la zona
    VZoff[i] = digitalRead(inputPinC[i]); // legge il pin digitale che rileva se la valvola è chiusa
    if (Zinoff[i] == false && Catt_VZ[i] > 0) { // il ciclo di chiusura valvola è minore di cicli previsti
      myTime6 = millis(); // azzera il tempo di chiusura della valvola
      Zinoff[i] = true; // la valvola della zona si sta chiudento
      digitalWrite(relePin[i + 9], LOW); // accende il relè che chiude la valvola di zona
      Zinoff[i] == true;
    } else if (Zinoff[i] && Catt_VZ[i] > 0 && millis() - myTime6 > 5000) { // ha eseguito il primo ciclo di chiusura e sono passati 5 secondi
      Catt_VZ[i] = Catt_VZ[i] - 1; // decrementa il ciclo di chiusura
      Zinoff[i] = false;
    } else if (VZoff[i]) {
      Catt_VZ[i] = 0;
      digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona     
    } else if (Catt_VZ[i] == 0) {
      digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona     
      erroreVZ_C[i] = true; // se ha superato i cicli previsti attiva l'errore di chiusura
    }
  }
}

void solaretermico() {
  oled.setCursor(0, 3);
  oled.print("R4:");
  if (pompaST) oled.print("on ");
  if (pompaST == false) oled.print("off");
  oled.print(" S2:");
  oled.print(Sonda[2 - 1]);
  oled.print("` S3:");
  oled.print(Sonda[3 - 1]);
  oled.print("` S5:");
  oled.print(Sonda[5 - 1]);
  if (erroreST) oled.print(" E");
  if (erroreST == false) oled.print("  ");
  if (pompaST == false && erroreST == false && Sonda[2 - 1] > Sonda[5 - 1]) {
    myTime2 = millis();
    digitalWrite(relePin[4 - 1], LOW);
    Serial.println("R4acceso");
    pompaST = true;
  }
  if (pompaST && Sonda[3 - 1] < Sonda[2 - 1]*0.9 && millis() - myTime2 > T_att_st) {
    // passato il tempo T_att_st se la temperatura di entrata al puffer basso è inferiore al 90% di quella dei pannelli viene segnalato un errore
    Serial.println("Errore solare termico");
    erroreST = true;
    Serial.println("R4spento");
    digitalWrite(relePin[4 - 1], HIGH);
    pompaST = false;
  }
  if (pompaST && Sonda[2 - 1] <= Sonda[5 - 1]) {
    Serial.println("R4spento");
    digitalWrite(relePin[4 - 1], HIGH);
    pompaST = false;
  }
}

void valvola3vie() {
  Serial.print("S1:");
  Serial.println(Sonda[0]);  
  Serial.print("S4:");
  Serial.println(Sonda[3]);
  TempV3vie = 30 + (20 - Sonda[1 - 1]) * 2 / 3; // imposta temperatura uscita valvola 3 vie in base alla tempratura esterna (50°C a -10°C e 30°C a 20°C);
  Serial.print("T3vie:");
  Serial.println(TempV3vie);
  oled.setCursor(0, 2);
  if (Valvola3vieA || Valv3vieinattA) oled.print("A ");
  if (Valvola3vieC || Valv3vieinattC) oled.print("C ");
  oled.print((int)((float)Catt_V3vie / (float)nc_V3vie * 100));
  oled.print(" S4:");
  oled.print(Sonda[4 - 1]);
  oled.print("` S6:");
  oled.print(Sonda[6 - 1]);
  oled.print("` S7:");
  oled.print(Sonda[7 - 1]);
  if (erroreIP) oled.print(" E");
  if (erroreIP == false) oled.print("  ");
  if (Sonda[6 - 1] > Sonda[4 - 1]) MessageRC = true;
  V3vieon = digitalRead(inputPinV3vieA); // legge il pin digitale che rileva se la valvola di zona è aperta
  V3vieoff = digitalRead(inputPinV3vieC); // legge il pin digitale che rileva se la valvola di zona è chiusa
  if (V3vieon == false && erroreIP == false && erroreV3vieA == false && Sonda[6 - 1] <  TempV3vie - BMTempV3vie / 2) {
    // se la valvola 3 vie non è aperta, non è in errore e la temperatura S6 è minore della temperatura impostata meno banda morta
    if (Valvola3vieA == false && Valv3vieinattA == false) {
      myTime3 = millis();
      myTime0 = millis();
      Serial.println("V3vie apre");
      digitalWrite(relePin[6 - 1], LOW); // accende il relè di apertura della valvola 3 vie
      Valvola3vieA = true; // la valvola a 3 vie è in fase di apertura
      Catt_V3vie = Catt_V3vie + 1; // aumenta il numero dei cicli di apertura
      if (V3vieon == false && Catt_V3vie > nc_V3vie) {
        erroreV3vieA = true; // se ha superato i cicli previsti attiva l'errore di apertura
        digitalWrite(relePin[6 - 1], HIGH); // spegne il relè di apertura della valvola 3 vie
        Serial.println("Errore di apertura valvola 3 vie");
      }
    } else if (Valv3vieinattA == false) {
      if (V3vieon || millis() - myTime3 > T_V3vie) {
        Serial.println("V3vie spenta");
        digitalWrite(relePin[6 - 1], HIGH);
        Valv3vieinattA = true;
        Valvola3vieA = false; // la valvola a 3 vie è ferma       
        myTime4 = millis();
      }
    } else {
      if (millis() - myTime4 > T_att_V3vie) {
        Valv3vieinattA = false;
      }
    }
  }
  if (V3vieoff == false && erroreIP == false && erroreV3vieC == false && Sonda[6 - 1] > TempV3vie + BMTempV3vie / 2) {
    // se la valvola 3 vie non è chiusa, in movimento, non è in errore e la temperatura S6 è maggiore della temperatura impostata più banda morta
    if (Valvola3vieC == false && Valv3vieinattC == false) {
      Serial.println("V3vie chiude");
      myTime3 = millis();
      myTime0 = millis();
      digitalWrite(relePin[5 - 1], LOW);
      Valvola3vieC = true; // la valvola a 3 vie è in fase di chiusura
      Catt_V3vie = Catt_V3vie - 1; // decrementa il numero dei cicli di apertura/chiusura
      if (V3vieoff == false && Catt_V3vie <= 0) {
        erroreV3vieC = true; // se ha superato i cicli previsti attiva l'errore
        digitalWrite(relePin[5 - 1], HIGH); // spegne il relè di chiusura valvola 3 vie
        Serial.println("Errore di chiusura valvola 3 vie");
      }
    } else if (Valv3vieinattC == false) {
      if (V3vieoff || millis() - myTime3 > T_V3vie) {
        Serial.println("V3vie spenta");
        digitalWrite(relePin[5 - 1], HIGH);
        Valv3vieinattC = true;
        Valvola3vieC = false; // la valvola a 3 vie è ferma       
        myTime4 = millis();
      }
    } else {
      if (millis() - myTime4 > T_att_V3vie) {
        Valv3vieinattC = false;
      }
    }
  }
  if ((Valvola3vieA || Valvola3vieC) && erroreIP == false && Sonda[7 - 1] < Sonda[6 - 1]*0.9 && millis() - myTime0 > T_att_ip) {
    // passato il tempo T_att_st se la temperatura di uscita valvola 3 vie è inferiore al 90% di quella di ritorno dell'impianto a pavimento
    Serial.println("Errore malfunzionamento pompa uscita valvola a 3 vie");
    erroreIP = true;
    Serial.println("R7spento");
    digitalWrite(relePin[7 - 1], HIGH);
  }
}

void acquasanitaria() {
  oled.setCursor(0, 3);
  oled.print("R3:");
  if (pompaAS) oled.print("on ");
  if (pompaAS == false) oled.print("off");
  oled.print(" S4:");
  oled.print(Sonda[4 - 1]);
  oled.print("` S8:");
  oled.print(Sonda[8 - 1]);
  oled.print("` S9:");
  oled.print(Sonda[9 - 1]);
  if (erroreAS) oled.print(" E");
  if (erroreAS == false) oled.print("  ");
  if (pompaAS == false && Sonda[8 - 1] < Sonda[9 - 1] + BMTempAS / 2 && Sonda[4 - 1] > TempPaMin && Sonda[9 - 1] < TempASMax) {
    // se la pompa acqua sanitaria non è in funzione e S4 maggiore S9 e S4 minore 25 e S8 minore TempASMax
    digitalWrite(relePin[3 - 1], LOW); // il relè 03 accende la pompaAS
    Serial.println("R3acceso");
    pompaAS = true; // pone vera pompaAs
  }
  if (pompaAS && Sonda[9 - 1] < Sonda[9 - 1]*0.75 && millis() - myTime8 > T_att_as) {
    // passato il tempo T_att_as se la temperatura di uscita dello scambiatore è inferiore al 75% di quella di entrata viene segnalato un errore
    Serial.println("Errore pompa acqua sanitaria");
    erroreAS = true;
    Serial.println("R3spento");
    digitalWrite(relePin[3 - 1], HIGH);
    pompaAS = false;
  }
  if (pompaAS && Sonda[9 - 1] == Sonda[8 - 1] - BMTempAS / 2) { // se la pompa è accesa e S08 uguale a S09 + BM/2
    Serial.println("R3spento");
    digitalWrite(relePin[3 - 1], HIGH); // il relè 03 spegne la pompaAS
    pompaAS = false; //pone falsa pompaAS
  }
}

void caldaia() {
  oled.setCursor(0, 4);
  oled.print("R2:");
  if (pompaCA) oled.print("on ");
  if (pompaCA == false) oled.print("off");
  oled.print(" S10:");
  oled.print(Sonda[10 - 1]);
  oled.print("` S11:");
  oled.print(Sonda[11 - 1]);
  oled.print("` S12:");
  oled.print(Sonda[12 - 1]);
  if (erroreCA) oled.print(" E");
  if (erroreCA == false) oled.print("  ");
  if (pompaCA == false && Sonda[10 - 1] > Sonda[4 - 1] &&  Sonda[10 - 1] > 50) {
    // se la pompa caldaia non è in funzione e S10 > S04 (temperatura caldaia maggiore della temperatura puffer alto e maggiore di 50°)
    myTime5 = millis();
    digitalWrite(relePin[2 - 1], LOW); // il relè 02 accende la pompa uscita caldaia
    Serial.println("R2acceso");
    pompaCA = true; // pone vera pompaCA
  }
  if (pompaCA && Sonda[11 - 1] < Sonda[4 - 1]*0.9 && millis() - myTime9 > T_att_ca) {
    // passato il tempo T_att_ca se la temperatura di uscita dalla pompa S11 è inferiore al 90% di quella di S4 (puffer alto) viene segnalato un errore
    Serial.println("Errore pompa caldaia");
    erroreCA = true;
    Serial.println("R2spento");
    digitalWrite(relePin[2 - 1], HIGH);
    pompaCA = false;
  }
  if (pompaCA && Sonda[10 - 1] < 50 && millis() - myTime5 > T_att_rc) {
    // se dopo T_att_rc e temperatura caldaia S10 minore di 50 ferma la pompa
    Serial.println("R2spento");
    digitalWrite(relePin[2 - 1], HIGH);
    pompaCA = false;
  }
}

void leggitemperature() {
  if (lettemp == false) { // legge le temperature da tutte le sonde ogni Tempriltemp millisecondi
    myTime7 = millis();
    lettemp = true;
    for (int i = 0; i < 5; i++) { // da 0 a 4 sono le schede RTD a 4 ingressi
      for (int j = 0; j < 4; j++) {
        digitalWrite(selPinA[i], dataPinAB[0] [j]); // seleziona le porte del multiplexer a 4 ingressi CD4052
        digitalWrite(selPinB[i], dataPinAB[1] [j]);
        delay(10); // ferma il programma per 10 millisecondi
        sensorValue = 0; // azzera la variabile
        for (int n = 0; n < 10; n++) { // legge i valori dai sensori 10 volte
          sensorValue = sensorValue + analogRead(sensorPin[i]); // legge il valore sull'ingresso analogico corrispondente A0-A4
        }
        Sonda[i * 4 + j] = (sensorValue/10 - 155) / 4.3; // calcola la media e la memorizza nella matrice Sonda
        Serial.print("Sonda");
        Serial.print(i * 4 + j + 1);
        Serial.print(":");
        Serial.println(Sonda[i * 4 + j]);
      }
    }
  } else if (lettemp == true && millis() - myTime7 > Tempriltemp) { // se è passato Tempriltemp
    lettemp = false;
  }
}

void chiude_V() {
  // display a line of text
  oled.setCursor(42,2);
  oled.print("Chiusura");
  oled.setCursor(27,3);
  oled.print("valvole di zona");
  TVZoff = false; // pone la variabile TVZoff a falso
  do {
    for (int i = 0; i < 4; i++) { // controlla se le valvole di zona sono chiuse
      VZoff[i] = digitalRead(inputPinC[i]); // legge il pin digitale che rileva se la valvola di zona è chiusa
      if (VZoff[i] == true) { // se la valvola di zona è chiusa
        digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvole di zona
      } else {
        digitalWrite(relePin[i + 9], LOW);
        delay(T_VZ); // aspetta il tempo di chiusura valvola di zona
        break; // nel caso una qualsiasi non lo sia esce dal ciclo for
      }
      TVZoff = true; // se tutte le valvole risultano chiuse pone TVZoff a vero
    }
  } while (TVZoff == false); // Se TVZoff è falso ripete il ciclo while
  oled.clear();
  oled.setCursor(42,2);
  oled.print("Chiusura");
  oled.setCursor(30,3);
  oled.print("valvola 3 vie");
  do {
    V3vieoff = digitalRead(inputPinV3vieC); // legge il pin digitale che rileva se la valvola di zona è chiusa
    if (V3vieoff) { // se la valvola 3 vie è chiusa
      digitalWrite(relePin[5 - 1], HIGH); // spegne il relè che chiude la valvola 3 vie
    } else {
      digitalWrite(relePin[5 - 1], LOW); // accende il relè che chiude la valvola 3 vie
      delay(T_V3vie); // aspetta il tempo di chiusura valvola 3 vie
    }
  } while (V3vieoff == false); // Se TVZoff è falso ripete il ciclo while
  oled.setCursor(45,3);
  oled.print("Finito!");
  delay(2000);
  oled.clear();
}

void createWebPage() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Client");
    bufferArgs = "";

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      while(client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // Here is where the POST data is. 
          while(client.available()) {
             char c = client.read();
             if (c != '&') {
               bufferArgs = bufferArgs + c;
             } else {
               separatearg();
               bufferArgs = "";         
             }
          }
          Serial.println("Sending response");
          // send a standard http response header
          client.println("HTTP/1.0 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<html>");
          client.println("<head><title>Impianto Termico</title>");
          client.println("<style>");
          client.println("body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}");
          client.println("</style>");
          client.println("</head>");
          client.println("<body>");
          client.println("<script type='text/javascript'> var timeleft = 60; var downloadTimer = setInterval(function(){ timeleft--; document.getElementById('countdowntimer').textContent = timeleft; if(timeleft <= 0) clearInterval(downloadTimer); },1000); </script>");
          client.println("<head> <meta http-equiv='refresh' content='60'> </head>");
          client.println("<h1>GESTIONE IMPIANTO TERMICO</h1>");
          client.println("<p>La pagina verr&agrave; aggiornata automaticamente fra <span id='countdowntimer'>10 </span> secondi</p>");
          client.println("<form ACTION=\"http://192.168.1.177\" METHOD=\"post\">");
          client.println("<input type='hidden' name='zonaP2' value='false'>");
          client.print("<input type='checkbox' name='zonaP2' value='true'");
          if (Zona[3]) client.print(" checked");
          client.println("> Selezionare la casella per attivare la Zona P.2</label><br>");
          client.println("<input type='hidden' name='zonaP1' value='false'>");
          client.print("<input type='checkbox' name='zonaP1' value='true'");
          if (Zona[2]) client.print(" checked");
          client.println("> Selezionare la casella per attivare la Zona P.1</label><br>");
          client.println("<input type='hidden' name='zonaPT' value='false'>");
          client.print("<input type='checkbox' name='zonaPT' value='true'");
          if (Zona[1]) client.print(" checked");
          client.println("> Selezionare la casella per attivare la Zona P.T</label><br>");
          client.println("<input type='hidden' name='zonaPint' value='false'>");
          client.print("<input type='checkbox' name='zonaPint' value='true'");
          if (Zona[0]) client.print(" checked");
          client.println("> Selezionare la casella per attivare la Zona P.int</label><br>");
          client.println("<font size='4'>Temperatura impianto pavimento P.2: <input type='number' value='" + String(TempPav[0]) + "' min='20' max='50' name='tempP2' style='text-align:right;'>");
          client.println(" Banda morta: <input type='number' value='" + String(BMTempPav[0]) + "' min='2' max='10' name='BMP2' style='text-align:right;'>");
          client.println(" Step di apertura/chiusura valvole di zona (secondi): <input type='number' value='" + String(T_VZ/1000) + "' min='1' max='30' name='TVZ' style='text-align:right;'><br>");
          client.println("Temperatura impianto pavimento P.1: <input type='number' value='" + String(TempPav[1]) + "' min='20' max='50' name='tempP1' style='text-align:right;'>");
          client.println(" Banda morta: <input type='number' value='" + String(BMTempPav[1]) + "' min='2' max='10' name='BMP1' style='text-align:right;'>");
          client.println(" Tempo totale di apertura/chiusura valvole di zona (secondi): <input type='number' value='" + String(TT_VZ/1000) + "' min='30' max='120' name='TTVZ' style='text-align:right;'><br>");
          client.println("Temperatura impianto pavimento P.T: <input type='number' value='" + String(TempPav[2]) + "' min='20' max='50' name='tempPT' style='text-align:right;'>");
          client.println(" Banda morta: <input type='number' value='" + String(BMTempPav[2]) + "' min='2' max='10' name='BMPT' style='text-align:right;'>");
          client.println(" Tempo di attesa prima della lettura della temperatura di uscita zona (secondi): <input type='number' value='" + String(T_att_ip/1000) + "' min='30' max='3000' name='TAIP' style='text-align:right;'><br>");
          client.println("Temperatura impianto pavimento P.int: <input type='number' value='" + String(TempPav[3]) + "' min='20' max='50' name='tempPint' style='text-align:right;'>");
          client.println(" Banda morta: <input type='number' value='" + String(BMTempPav[3]) + "' min='2' max='10' name='BMPint' style='text-align:right;'><br>");
          client.println("Banda morta valvola 3 vie: <input type='number' value='" + String(BMTempV3vie) + "' min='2' max='10' name='BMV3vie' style='text-align:right;'><br>");
          client.println("Step di apertura/chiusura valvola 3 vie (secondi): <input type='number' value='" + String(T_V3vie/1000) + "' min='1' max='30' name='TV3vie' style='text-align:right;'>");
          client.println(" Tempo totale di apertura/chiusura valvola 3 vie (secondi): <input type='number' value='" + String(TT_V3vie/1000) + "' min='30' max='120' name='TTV3vie' style='text-align:right;'><br>");
          client.println("Tempo di attesa prima della rilettura della temperatura di uscita della valvola 3 vie (secondi): <input type='number' value='" + String(T_att_V3vie/1000) + "' min='30' max='3000' name='TAV3vie' style='text-align:right;'><br>");
          client.println("Tempo di attesa prima della rilettura della temperatura di ritorno impianti a pavimento (secondi): <input type='number' value='" + String(T_att_ip/1000) + "' min='30' max='3000' name='TAIP' style='text-align:right;'><br>");
          client.println("Tempo di attesa prima della rilettura della temperatura di ritorno solare termico (secondi): <input type='number' value='" + String(T_att_st/1000) + "' min='30' max='3000' name='TAST' style='text-align:right;'><br>");
          client.println("Temperatura massima acqua calda sanitaria: <input type='number' value='" + String(TempASMax) + "' min='40' max='55' name='tempAS' style='text-align:right;'>");
          client.println(" Banda morta: <input type='number' value='" + String(BMTempAS) + "' min='2' max='10' name='BMPAS' style='text-align:right;'><br>");
          client.println("Tempo di attesa prima della lettura della temperatura di uscita dello scambiatore acqua calda sanitaria (secondi): <input type='number' value='" + String(T_att_as/1000) + "' min='30' max='3000' name='TAAS' style='text-align:right;'><br>");
          client.println("Tempo di attesa prima della rilettura della temperatura della caldaia (secondi): <input type='number' value='" + String(T_att_rc/1000) + "' min='30' max='3000' name='TARC' style='text-align:right;'><br>");
          client.println("Tempo di attesa prima della lettura della temperatura di ritorno dal puffer alla caldaia (secondi): <input type='number' value='" + String(T_att_ca/1000) + "' min='30' max='3000' name='TACA' style='text-align:right;'><br>");
          client.println("Tempo di rilettura delle temperature rilevate dalle sonde (secondi): <input type='number' value='" + String(Tempriltemp/1000) + "' min='30' max='3000' name='TRST' style='text-align:right;'><br>");
          client.println("<INPUT TYPE=\"SUBMIT\" NAME=\"submit\" VALUE=\"Salva impostazioni\">");
          client.println("<h1>DATI IMPIANTO TERMICO</h1>");  
          client.println("<b>IMPIANTO A PAVIMENTO:</b><br>");  
          String szona1, szona2, szona3, szona4, sVZa1, sVZa2, sVZa3, sVZa4, sVZc1, sVZc2, sVZc3, sVZc4;
          if (Zona[0]) szona1 = "on "; else szona1 = "off";
          if (Zona[1]) szona2 = "on "; else szona2 = "off";
          if (Zona[2]) szona3 = "on "; else szona3 = "off";
          if (Zona[3]) szona4 = "on "; else szona4 = "off";
          if (VzonaA[0]) sVZa1 = "apre "; else sVZa1 = "";
          if (VzonaA[1]) sVZa2 = "apre "; else sVZa2 = "";
          if (VzonaA[2]) sVZa3 = "apre "; else sVZa3 = "";
          if (VzonaA[3]) sVZa4 = "apre "; else sVZa4 = "";
          if (VzonaC[0]) sVZc1 = "chiude "; else sVZc1 = "";
          if (VzonaC[1]) sVZc2 = "chiude "; else sVZc2 = "";
          if (VzonaC[2]) sVZc3 = "chiude "; else sVZc3 = "";
          if (VzonaC[3]) sVZc4 = "chiude "; else sVZc4 = "";
          String seazona1, seazona2, seazona3, seazona4;
          if (erroreVZ_A[0]) seazona1 = "Errore apertura valvola "; else seazona1 = "";
          if (erroreVZ_A[1]) seazona2 = "Errore apertura valvola "; else seazona2 = "";
          if (erroreVZ_A[2]) seazona3 = "Errore apertura valvola "; else seazona3 = "";
          if (erroreVZ_A[3]) seazona4 = "Errore apertura valvola "; else seazona4 = "";
          String seczona1, seczona2, seczona3, seczona4;
          if (erroreVZ_C[0]) seczona1 = "Errore chiusura valvola "; else seczona1 = "";
          if (erroreVZ_C[1]) seczona2 = "Errore chiusura valvola "; else seczona2 = "";
          if (erroreVZ_C[2]) seczona3 = "Errore chiusura valvola "; else seczona3 = "";
          if (erroreVZ_C[3]) seczona4 = "Errore chiusura valvola "; else seczona4 = "";
          for (int i = 0; i < 4; i++) {
            paVZ[i] = Catt_VZ[i] / nc_VZ * 100; // calcola la percentuale di apertura delle valvole di zona
          }
          paV3vie = (int)((float)Catt_V3vie / (float)nc_V3vie * 100); // calcola la percentuale di apertura/chiusura   delle valvole di zona
          client.println("Zona 4 (P.2): " + String(Sonda[16 - 1]) + "&deg;C " + szona4 + sVZa4 + sVZc4 + paVZ[3] + "&#37; <font color='red'>" + seazona4 + seczona4 + "</font>- Zona 3 (P.1): " + String(Sonda[15 - 1]) + "&deg;C " + szona3 + sVZa3 + sVZc3 + " " + paVZ[2] + "&#37; <font color='red'>" + seazona3 + seczona3 + "</font>- Zona 2 (P.T): " + String(Sonda[14 - 1]) + "&deg;C " + szona2 + sVZa2 + sVZc2 + " " + paVZ[1] + "&#37; <font color='red'>" + seazona2 + seczona2 + "</font>- Zona 1 (P.int)1: " + String(Sonda[13 - 1]) + "&deg;C " + szona1 + sVZa1 + sVZc1 + " " + paVZ[0] + "&#37; <font color='red'>" + seazona1 + seczona1 + "</font><br>");
          client.println("<b>VALVOLA A 3 VIE:</b><br>");  
          client.println("Temperatura esterna: " + String(Sonda[1 - 1]) + "&deg;C - Temperatura calcolata alimentazione impianto a pavimento: " + String(TempV3vie) + "&deg;C</br>");
          String sValvola3vie, sValvola3viea, sValvola3viec, serroreV3viea, serroreV3viec,serroreIP;
          if (Valvola3vieA || Valv3vieinattA) sValvola3viea = "apre "; else sValvola3viea = "";
          if (Valvola3vieC || Valv3vieinattC) sValvola3viec = "chiude "; else sValvola3viec= "";
          if (erroreV3vieA) serroreV3viea = "Errore apertura valvola 3 vie"; else serroreV3viea = "";
          if (erroreV3vieC) serroreV3viec = "Errore chiusura valvola 3 vie"; else serroreV3viec = "";
          if (erroreIP) serroreIP = "Errore malfunzionamento pompa uscita valvola a 3 vie"; else serroreIP = "";
          client.println(sValvola3vie + sValvola3viea + sValvola3viec + paV3vie + "&#37; <font color='red'>" + serroreV3viea + serroreV3viec + "</font> - Temperatura puffer alto: " + String(Sonda[4 - 1]) + "&deg;C - Temperatura uscita pompa: " + String(Sonda[6 - 1]) + "&deg;C - Temperatura ritorno da impianti a pavimento: " + String(Sonda[7 - 1]) + "&deg; <font color='red'>" + serroreIP +"</font><br>");
          client.println("<b>SOLARE TERMICO:</b><br>");  
          String spompaST, seST;
          if (pompaST) spompaST = "on "; else spompaST = "off";
          if (erroreST) seST = "Errore malfunzionamento pompa solare termico"; else seST = "";
          client.println("Pompa R4: " + spompaST + " - Temperatura puffer basso: " + String(Sonda[5 - 1]) + "&deg;C - Temperatura pannelli: " + String(Sonda[2 - 1]) + "&deg;C - Temperatura entrata puffer: " + String(Sonda[3 - 1]) + "&deg; <font color='red'>" + seST +"</font><br>");
          client.println("<b>ACQUA CALDA SANITARIA:</b><br>");  
          String spompaAS, seAS;
          if (pompaAS) spompaAS = "on "; else spompaAS = "off";
          if (erroreAS) seAS = "Errore malfunzionamento pompa acqua calda sanitaria"; else seAS = "";
          client.println("Pompa R3: " + spompaAS + " - Temperatura puffer alto: " + String(Sonda[4 - 1]) + "&deg;C - Temperatura ingresso scambiatore: " + String(Sonda[8 - 1]) + "&deg;C - Temperatura uscita scambiatore: " + String(Sonda[9 - 1]) + "&deg; <font color='red'>" + seAS +"</font><br>");
          client.println("<b>CALDAIA:</b><br>");  
          String spompaCA, seCA;
          if (pompaCA) spompaCA = "on "; else spompaCA = "off";
          if (erroreCA) seCA = "Errore malfunzionamento pompa caldaia"; else seCA = "";
          client.println("Pompa R2: " + spompaCA + " - Temperatura caldaia: " + String(Sonda[10 - 1]) + "&deg;C - Temperatura uscita pompa: " + String(Sonda[11 - 1]) + "&deg;C - Temperatura ritorno da puffer alto: " + String(Sonda[12 - 1]) + "&deg; <font color='red'>" + seCA +"</font><br>");
          //client.println("<meta http-equiv='refresh' content='1'>");
          client.println("</form>");
          client.println("</body>");
          client.println("</html>");
          client.stop();
        }
        else if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    Serial.println("Disconnected");
  }
}

void separatearg() {
  boolean farg = false; // è stata trovata la fine di arg (nome campo)
  value = ""; // svuota le variabili
  arg = "";
  Serial.println(bufferArgs);
  for(int j = 0; j <= bufferArgs.length(); j++) { // scorri i caratteri da ii (primo del gruppo arg=contenuto)
    if (bufferArgs.substring(j, j + 1) == "=") { // se trovi il carattere =
      farg = true; // è stata trovata la fine di arg
    } else {
      if (farg) { // se farg è vero
        value += bufferArgs.substring(j, j + 1); // aggiungi il carattere a value (contenuto)
      } else { // altrimenti
          arg += bufferArgs.substring(j, j + 1); // aggiungi il carattere a arg (nome campo)
      }
    }
  }
  Serial.println(arg);
  Serial.println(value);
  if (arg == "zonaP2") {
    if (value == "true") {
      Zona[3] = true;
    } else {
      Zona[3] = false;
    }
  }
  if (arg == "zonaP1") {
    if (value == "true") {
      Zona[2] = true;
    } else {
      Zona[2] = false;
    }
  }
  if (arg == "zonaPT") {
    if (value == "true") {
      Zona[1] = true;
    } else {
      Zona[1] = false;
    }
  }
  if (arg == "zonaPint") {
    if (value == "true") {
      Zona[0] = true;
    } else {
      Zona[0] = false;
    }
  }
  if (arg == "tempP2") TempPav[3] = value.toInt();
  if (arg == "tempP1") TempPav[2] = value.toInt();
  if (arg == "tempPT") TempPav[1] = value.toInt();
  if (arg == "tempPint") TempPav[0] = value.toInt();
  if (arg == "BMP2") BMTempPav[3] = value.toInt();
  if (arg == "BMP1") BMTempPav[2] = value.toInt();
  if (arg == "BMPT") BMTempPav[1] = value.toInt();
  if (arg == "BMPint") BMTempPav[0] = value.toInt();
  if (arg == "TVZ") T_VZ = value.toInt() * 1000;
  if (arg == "TTVZ") TT_VZ = value.toInt() * 1000;
  if (arg == "TAIP") T_att_ip = value.toInt() * 1000;
  if (arg == "TAST") T_att_st = value.toInt() * 1000;
  if (arg == "TV3vie") T_V3vie = value.toInt() * 1000;
  if (arg == "TTV3vie") TT_V3vie = value.toInt() * 1000;
  if (arg == "TAV3vie") T_att_V3vie = value.toInt() * 1000;
  if (arg == "BMV3vie") BMTempV3vie = value.toInt();
  if (arg == "TAIR") T_att_ir = value.toInt() * 1000;
  if (arg == "tempAS") TempASMax = value.toInt();
  if (arg == "BMPas") BMTempAS = value.toInt();
  if (arg == "TARC") T_att_rc = value.toInt() * 1000;
  if (arg == "TAAS") T_att_as = value.toInt() * 1000;
  if (arg == "TACA") T_att_ca = value.toInt() * 1000;
  if (arg == "TRST") {
    Tempriltemp = value.toInt() * 1000;
    salvaDati();
  }
}

void salvaDati() {
  Serial.println("salva dati");
  SD.remove("dati.txt"); // cancella il file
  if (SD.exists("dati.txt")) {
    Serial.println("dati.txt exists.");
  } 
  myFile = SD.open("dati.txt", FILE_WRITE); // apre il file
  if (myFile) {  // se il file esiste
    myFile.println(Zona[0]); // scrive i dati
    myFile.println(Zona[1]);
    myFile.println(Zona[2]);
    myFile.println(Zona[3]);
    myFile.println(TempPav[0]);
    myFile.println(TempPav[1]);
    myFile.println(TempPav[2]);
    myFile.println(TempPav[3]);
    myFile.println(BMTempPav[0]);     
    myFile.println(BMTempPav[1]);     
    myFile.println(BMTempPav[2]);     
    myFile.println(BMTempPav[3]);
    myFile.println(T_VZ / 1000);
    myFile.println(TT_VZ / 1000);
    myFile.println(T_att_ip / 1000);
    myFile.println(T_att_st / 1000);
    myFile.println(T_V3vie / 1000);
    myFile.println(TT_V3vie / 1000);
    myFile.println(T_att_V3vie / 1000);
    myFile.println(BMTempV3vie);
    myFile.println(T_att_ir / 1000);
    myFile.println(TempASMax);
    myFile.println(BMTempAS);
    myFile.println(T_att_rc / 1000);
    myFile.println(T_att_as / 1000);
    myFile.println(T_att_ca / 1000);
    myFile.println(Tempriltemp /1000);
    myFile.close(); // chiude il file:
    Serial.println("Scritto");
  }
}

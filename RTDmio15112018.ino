/*Creato da Giuseppe Tamanini
 * 15/11/2018
 * Licenza CC BY-NC-SA 3.0 IT
 */

//MISO - pin 50
//MOSI - pin 51
//CLK  - pin 52
//CS   - pin 53

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// OLED display TWI address
#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(-1);

int sensorPin[5] = {A0, A1, A2, A3, A4}; // pin analogico uscita RTD x 4 ingressi PT1000
int selPinA[5] = {38, 40, 42, 44, 46};   // pin A di commutazione dei multiplexer
int selPinB[5] = {39, 41, 43, 45, 47};   // pin B di commutazione dei multiplexer
// pin 38 e 39 prima scheda RTD per sonde S01 - S04
// pin 40 e 41 seconda scheda RTD per sonde S05 - S08
// pin 42 e 43 terza scheda RTD per sonde S09 - S12
// pin 44 e 45 quarta scheda RTD per sonde S13 - S16
// pin 46 e 47 quinta scheda RTD per sonda S17 - S20
int inputPin[4] = {2, 3, 4, 5}; // pin digitali in ingresso che leggono se le valvole di zona sono chiuse
// pin 2 valvola P.int
// pin 3 valvola P.T
// pin 4 valvola P.1
// pin 5 valvola P.2
boolean dataPinAB[2] [4] = {{1, 0, 0, 1}, {0, 1, 0, 1}}; // dati per la commutazione dei multiplexer
// 10 seleziona l'ingresso 1 della scheda RTD
// 01 seleziona l'ingresso 2 della scheda RTD
// 00 seleziona l'ingresso 3 della scheda RTD
// 11 seleziona l'ingresso 4 della scheda RTD
int relePin[16] = {22, 24, 26, 28, 30, 32, 34, 36, 23, 25, 27, 29, 31, 33, 35, 37};   // pin dove sono collegati i relè
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
// Sonda 08 temperatura uscita boiler acqua calda sanitaria
// Sonda 09 temperatura entrata boiler acqua calda sanitaria
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
boolean Vzona[4]; // una delle valvole di zona è in funzione?
boolean Pinatt[4]; // è in attesa della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
boolean Zinoff[4]; // è in chiusura della valvola di zona per la disattivazione della zona
int Catt_VZ[4] = {0, 0, 0, 0}; // ciclo attuale della valvola di zona
boolean VZoff[4]; // la valvola di zona è chiusa
boolean erroreVZ[4]; // la valvola di zona non si è chiusa
boolean TVZoff; // tutte le valvole di zona sono chiuse?
boolean pompaST; // la pompa del solare termico è in funzione?
boolean erroreST; // il solare termico è in errore
boolean Valvola3vie; // la valvola a 3 vie è in funzione?
boolean Valv3vieinatt; // è in attesa della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
int Catt_V3vie = 0; // ciclo attuale della valvola 3vie
boolean pompaAS; // la pompa dello scambiatore dell'acqua sanitaria è in funzione?
boolean pompaCA; // la pompa uscita caldaia è in funzione
boolean rele07; // il rele07 che alimenta la pompa in uscita dalla valvola a 3vie è accesso?
boolean lettemp; // ha già letto le temperature?

int TempPav = 35; // temperatura ritorno impianto a pavimento
int BMTempPav = 4; // tolleranza Temperatura impianto a pavimento
int T_VZ = 5000; // tempo in millisecondi (5 secondi) di apertura/chiusura valvola di zona dell'impianto a pavimento
int TT_VZ = 30000; // tempo in millesecondi per la totale apertura/chiusura valvola di zona
int T_att_ip = 5000; // tempo in millisecondi (30 secondi) di attesa prima della rilettura della temperatura di ritorno di una delle zone dell'impianto a pavimento
int T_att_st = 30000; // tempo in millisecondi (3 minuti) di attesa prima della lettura della temperatura di ritorno S03 del solare termico
int TempV3vie; // temperatura uscita valvola a 3 vie (verrà calcolata dal programma in base alla temperatura esterna)
int T_V3vie = 5000; // tempo in millisecondi (5 secondi) di apertura/chiusura valvola a 3 vie
int TT_V3vie = 30000; // tempo in millisecondi di totale apertura/ciusura valvola a 3 vie
int T_att_V3vie = 30000; // tempo in millisecondi (3 minuti) di attesa prima della rilettura della temperatura di uscita dalla valvola a 3 vie
int BMTempV3vie = 4; // tolleranza Temperatura uscita valvola 3 vie
int T_att_pc = 30000; // tempo in millisecondi (3 minuti) di attesa prima della lettura della temperatura di ritorno della caldaia
int BMTempAS = 2; // tolleranza Temperatura acqua sanitaria
int Tempriltemp = 5000; // tempo in millisecondi di rilettura delle temperature
int nc_VZ; // numero dei cicli di apertura/chiusura delle valvole di zona
int nc_V3vie; // numero dei cicli di apertura/chiusura delle valvola 3 vie

const int TempPavMin = 6; // allarme di TempPav minima (soglia antigelo)
const int TempPavMax = 50; // allarme di TempPav massima
const int TempASMax = 55; // temperatura massima acqua sanitaria

static unsigned long myTime; // tempo di apertura/chiusura valvole pavimento
static unsigned long myTime1; // tempo di attesa rilettura valvole pavimento
static unsigned long myTime2; // tempo di attesa lettura S3 del solare termico
static unsigned long myTime3; // tempo di apertura/chiusura valvola 3 vie
static unsigned long myTime4; // tempo di attesa rilettura S6 uscita valvola a 3 vieZona[i]
static unsigned long myTime5; // tempo di attesa lettura S13 ritorno caldaia
static unsigned long myTime6; // tempo di chiusura valvole pavimento disattivazione zone
static unsigned long myTime7; // tempo di attesa della rilettura delle temperature

String inputString = ""; // Stringa in entrata sulla seriale
bool stringComplete = false;  // la stringa è completata?

void setup() {
  Serial.begin(115200);
  analogReference(INTERNAL2V56);
  inputString.reserve(50);
  
  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();
  
  // declare the selPin as an OUTPUT:
  for (int i = 0; i < 5 ; i++) {
    pinMode(inputPin[i], INPUT);
    pinMode(selPinA[i], OUTPUT);
    pinMode(selPinB[i], OUTPUT);    
  }
  for (int i = 0; i < 16; i++) {
    pinMode(relePin[i], OUTPUT);
    digitalWrite(relePin[i], HIGH);
  }
  for (int i = 0; i < 4 ; i++) {
    Zona[i] = true;
    VZoff[i] = false;
  }
  nc_VZ = TT_VZ / 5000;
  if (nc_VZ * 5000 < TT_VZ) nc_VZ = nc_VZ + 1; // calcola il numero dei cicli che servono per completare l'apertura/chiusura delle valvole di zona
  nc_V3vie = TT_VZ / 5000;
  if (nc_V3vie * 5000 < TT_VZ) nc_V3vie = nc_V3vie + 1; // calcola il numero dei cicli che servono per completare l'apertura/chiusura della valvola 3 vie
  chiude_VZ();
}

void loop() {
  if (stringComplete) {
    inputString=inputString.substring(0,5);
    Serial.println(inputString);
    if (inputString == "Z0off") {
      Zona[0] = false;
      VZoff[0] = false;
      Serial.println("Zona 0 in spegnimento");
      inputString="";
      stringComplete = false;
    }
  }
  leggitemperature();
  for (int i = 0; i < 4; i++) { 
    pavimento(i);
  }
  for (int i = 0; i < 4; i++) { 
    if (Pinatt[i] == true && rele07 == false) {
      digitalWrite(relePin[7 - 1], LOW);; // accende il relè 07 che comanda la pompa uscita valvola 3 vie
      rele07 = true;
      Serial.println("R7acceso");
      break;
    }
  }
  TVZoff = true; // pone la variabile TVZoff a vero
  for (int i = 0; i < 4; i++) { // controlla se le valvole di zona sono chiuse 
    VZoff[i] = digitalRead(inputPin[i]); // legge il pin digitale che rileva se la valvola di zona è chiusa
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
    digitalWrite(relePin[7 - 1], HIGH); // spegne il relè 07 che comanda la pompa uscita valvola 3 vie
    Serial.println("R7spento");
    rele07 = false;
  }
  solaretermico();
  valvola3vie();
  acquasanitaria();
  caldaia();
  delay(5000);
}

void pavimento(int i) {
  Serial.print("S");
  Serial.print(i + 13);
  Serial.print(":");
  Serial.println(Sonda[i + 12]);
  if (Zona[i] && VZoff[i] == false && erroreVZ[i] == false && Sonda[i + 12] < TempPav - BMTempPav / 2) { // se la zona è attiva e la temperatura di ritorno della zona è minore di quella impostata - banda morta
    if (Vzona[i] == false && Pinatt[i] == false) {
      myTime = millis(); // azzera il tempo di apertura della valvola
      Serial.print("VZ");
      Serial.print(i);
      Serial.println("apre");
      Serial.println(Catt_VZ[i]);
      digitalWrite(relePin[i + 10], LOW); // accende il relè che apre la valvola di zona
      Vzona[i] = true; // a valvola della zona è in funzione
      Catt_VZ[i] = Catt_VZ[i] + 1; // decrementa il numero dei cicli di apertura/chiusura
      if (Catt_VZ[i] > nc_VZ) {
        erroreVZ[i] = true; // se ha superato i cicli previsti attiva l'errore
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
  VZoff[i] = digitalRead(inputPin[i]);
  if (Zona[i] && VZoff[i] == false && erroreVZ[i] == false && Sonda[i + 12] > TempPav + BMTempPav / 2) { // se la zona è attiva e la temperatura di ritorno della zona è maggiore di quella impostata + banda morta
    if (Vzona[i] == false && Pinatt[i] == false) {
      Serial.print("VZ");
      Serial.print(i);
      Serial.println("chiude");
      Serial.println(Catt_VZ[i]);
      myTime = millis(); // azzera il tempo di chiusura della valvola
      digitalWrite(relePin[i + 9], LOW); // accende il relè che chiude la valvola di zona
      Vzona[i] = true; // la valvola della zona è in funzione
      Catt_VZ[i] = Catt_VZ[i] - 1; // decrementa il numero dei cicli di apertura/chiusura
      if (Catt_VZ[i] == 0) {
        erroreVZ[i] = true; // se ha superato i cicli previsti attiva l'errore
        Serial.print("Errore di chiusura valvola di zona: ");
        Serial.println(i);
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
  if (Zona[i] == false && VZoff[i] == false && erroreVZ[i] == false) { // disattiva la zona
    VZoff[i] = digitalRead(inputPin[i]);
    if (Zinoff[i] == false && Catt_VZ[i] > 0) { // il ciclo di chiusura valvola è minore di 5 (deve fare 6 cicli)
      myTime6 = millis(); // azzera il tempo di chiusura della valvola
      Zinoff[i] = true; // la valvola della zona si sta chiudento
      digitalWrite(relePin[i + 9], LOW); // accende il relè che chiude la valvola di zona
      Zinoff[i] == true;
    } else if (Zinoff[i] && Catt_VZ[i] > 0 && millis() - myTime6 > 5000) { // ha eseguito il primo ciclo di chiusura e sono passati 5 secondi
      Catt_VZ[i] = Catt_VZ[i] - 1; // incrementa il ciclo di chiusura
      Zinoff[i] = false;
    } else if (VZoff[i]) {
      Catt_VZ[i] = 0;
      digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona     
    } else if (Catt_VZ[i] == 0) {
      digitalWrite(relePin[i + 9], HIGH); // spegne il relè che chiude la valvola di zona     
      erroreVZ[i] = true; // se ha superato i cicli previsti attiva l'errore
    }
  }
}

void solaretermico() {
  if (pompaST == false && erroreST == false && Sonda[2 - 1] > Sonda[5 - 1]) {
    myTime2 = millis();
    digitalWrite(relePin[4 - 1], LOW);
    Serial.println("R4acceso");
    pompaST = true;
  }
  if (pompaST && Sonda[3 - 1] < Sonda[2 - 1]*0.9 && millis() - myTime2 > T_att_st) {
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
  if (Sonda[4 - 1] <  TempV3vie - BMTempV3vie / 2) {
    if (Valvola3vie == false && Valv3vieinatt == false) {
      myTime3 = millis();
      Serial.println("V3vie apre");
      digitalWrite(relePin[6 - 1], LOW);
      Valvola3vie = true;
    } else if (Valv3vieinatt == false) {
      if (millis() - myTime3 > T_V3vie) {
        Serial.println("V3vie spenta");
        digitalWrite(relePin[6 - 1], HIGH);
        Valv3vieinatt = true;
        Valvola3vie = false;
        myTime4 = millis();
      }
    } else {
      if (millis() - myTime4 > T_att_ip) { 
        Valv3vieinatt = false;
      }
    }
  }
  if (Sonda[4 - 1] > TempV3vie + BMTempV3vie / 2) {
    if (Valvola3vie == false && Valv3vieinatt == false) {
      Serial.println("V3vie chiude");
      myTime3 = millis();
      digitalWrite(relePin[5 - 1], LOW);
      Valv3vieinatt = true;
    } else if (Valv3vieinatt == false) {
      if (millis() - myTime4 > T_V3vie) {
        Serial.println("V3vie spenta");
        digitalWrite(relePin[5 - 1], HIGH);
        Valv3vieinatt = true;
        Valvola3vie = false;
        myTime4 = millis();
      }
    } else {
      if (millis() - myTime4 > T_att_V3vie) { 
        Valv3vieinatt = false;
      }
    }
  }
}

void acquasanitaria() {
  if (pompaAS == false && Sonda[9 - 1] < Sonda[8 - 1] + BMTempAS / 2 && Sonda[8 - 1] < TempASMax) {
    // se la pompa acqua sanitaria non è in funzione e S09 < S08 - BM/2 e S08 < Temperatura massima acqua sanitaria (55°)
    digitalWrite(relePin[3 - 1], LOW); // il relè 03 accende la pompaAS
    Serial.println("R3acceso");
    pompaAS = true; // pone vera pompaAs
  }
  if (pompaAS && Sonda[8 - 1] == Sonda[9 - 1] - BMTempAS / 2) { // se la pompa è accesa e S08 uguale a S09 + BM/2
    Serial.println("R3spento");
    digitalWrite(relePin[3 - 1], HIGH); // il relè 03 spegne la pompaAS
    pompaAS = false; //pone falsa pompaAS
  }
}

void caldaia() {
  if (pompaCA == false && Sonda[10 - 1] > Sonda[4 - 1]) {
    // se la pompa caldaia non è in funzione e S10 > S04 (temperatura caldaia maggiiore della temperatura puffer alto)
    myTime5 = millis();
    digitalWrite(relePin[2 - 1], LOW); // il relè 02 accende la pompa uscita caldaia
    Serial.println("R2acceso");
    pompaCA = true; // pone vera pompaCA
  }
  if (pompaCA && Sonda[11 - 1] > 50 &&  Sonda[12 - 1] < 80 && millis() - myTime5 > T_att_pc) {
    // se dopo T_att_pc ??????????
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
        delay(100); // ferma il programma per 100 millisecondi
        sensorValue = 0; // azzera la variabile
        for (int n = 0; n < 10; n++) { // legge i valori dai sensori 10 volte
          sensorValue = sensorValue + analogRead(sensorPin[i]); // legge il valore sull'ingresso analogico corrispondente A0-A4
        }
        Sonda[i * 4 + j] = (sensorValue/10 - 272) / 4.55; // calcola la media e la memorizza nella matrice Sonda
      }
    }
  } else if (lettemp == true && millis() - myTime7 > Tempriltemp) { // se è passato Tempriltemp
    lettemp = false;
  }
}

void chiude_VZ() {
  // display a line of text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(40,20);
  display.print("Chiusura");
  display.setCursor(15,30);
  display.print("valvole di zona");

  // update display with all of the above graphics
  display.display();
  
  for (int i = 0; i < 4 ; i++) { // chiude le valvole di zona finché non sono rilevate tali 
    digitalWrite(relePin[i + 9], LOW);
  }
  TVZoff = false; // pone la variabile TVZoff a falso
  do {
    for (int i = 0; i < 4; i++) { // controlla se le valvole di zona sono chiuse 
      delay(1000); // aspetta un secondo
      VZoff[i] = digitalRead(inputPin[i]); // legge il pin digitale che rileva se la valvola di zona è chiusa
      if (VZoff[i] == false) break; // nel caso una qualsiasi non lo sia esce dal ciclo for
      TVZoff = true; // se tutte le valvole risultano chiuse pone TVZoff a vero
    }
  } while (TVZoff == false); // Se TVZoff è falso ripete il ciclo while
  for (int i = 0; i < 4 ; i++) { // spegne i relè che chiudono le valvole di zona
    digitalWrite(relePin[i + 9], HIGH);
  }
  display.clearDisplay();
  display.setCursor(40,25);
  display.print("Finito!");
  display.display();
}

void serialEvent() {
  while (Serial.available()) { // attende un nuovo carattere sulla seriale
    char inChar = (char)Serial.read(); // lo legge e lo memorizza in inChar
    inputString += inChar; // lo aggiunge a inputString:
    if (inChar == '\n') {  // se arriva un carettare di nuova linea
      stringComplete = true; // pone a vera la variabile stringComplete
    }
  }
}

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <Wire.h>

//MPU6050 akcelerometar i ziroskopski senzor
Adafruit_MPU6050 mpu;

#define PIXEL_PIN    13  // Digitalni pin na koji se vezuje LED traka(Data IN)

#define PIXEL_COUNT 32  // Broj dioda na traci(definisan broj u diagram.json)

// Deklaracija Neostrip objekta
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Broj "pixela" (individualnih dioda) na traci
// Argument 2 = Arduino pin #
// Argument 3 = flegovi koji opisuju tip LED dioda:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

//pinovi za tastere - povezani su na pinovima 12 i 4
#define BUTTON_PIN_12 12

//definisemo na kom pinu su ECHO i TRIG
#define ECHO_PIN 2 //digitalna dvojka
#define TRIG_PIN 3 //digitalna trojka


void setup() {
  // put your setup code here, to run once:
  //koristimo serijski port za komunikaciju sa baud rate-om 115200 (jer akcelerometar+ziroskop rade u ovom rezimu)
  Serial.begin(115200);

  while (!mpu.begin()) {
    Serial.println("MPU6050 not connected!");
    delay(1000);
  }
  Serial.println("MPU6050 ready!");

  //digitalni pin 4 i 12 su tasteri
  pinMode(BUTTON_PIN_12, INPUT_PULLUP);
  
  //Digitalni pin 3 je OUTPUT
  pinMode(TRIG_PIN, OUTPUT);
  //Difitalni pin 2 je INPUT
  pinMode(ECHO_PIN, INPUT);

  strip.begin(); // Inicijalizacija NeoStrip LED trake
  strip.show();  // ovim zapravo renderujemo osvetljenje(posto ga nismo podesili bice black u pocetku)

}

//dogadjaji koje detektuje senzor
sensors_event_t event;
boolean is_turned_on = false;
boolean was_high = false;

void colorWipe(uint32_t color); // funkcija koja boji sve diode specificiranom bojom
int mySpecialNumber( sensors_event_t a,sensors_event_t g,sensors_event_t temp); // "magicni" broj
float readDistanceCM(); //cita distancu objekta od ultrasonicnog senzora distance u santimetrima

void loop() {
  // put your main code here, to run repeatedly:

  //citamo DIGITALNU vrednost sa dugmeta koje je vezano na PIN 12 
  int value = digitalRead((BUTTON_PIN_12));

  //citamo distancu sa proximity senzora
  float distance = readDistanceCM();
  //ukoliko je nesto na manje od 5 cm od senzora, detektujemo ga kao true
  bool isNearby = distance < 5;

  //Dohvata acceleration(a), gyroscope(g)
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if(value == HIGH)
    was_high = true;

  if(value == LOW && was_high){
    was_high = false;
    is_turned_on = !is_turned_on;
  }

  //opseg boja [0-16,777,215] brojcano
  if(is_turned_on){
    colorWipe( 
    isNearby ? 
    strip.Color(255,255,255):2277000);

     //frekventni opseg [50, 125]
    tone( 8, 
      isNearby ? 
      250:mySpecialNumber(a,g, temp), 
    250);

  }else{
    //ako je uredjaj ugasen boja je "crna", a ton se ne cuje
    colorWipe(  strip.Color(0,0,0));
    tone( 8, 0, 250);
  }

  delay(10);
  
}

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color) {
  for(int i=0; i<strip.numPixels(); i++) { // For svaki piksel na traci...
    strip.setPixelColor(i, color);         //  setuj boju 'color' i-tog piksela
    strip.show();                          //  Renderuj promenu
  }
}

int mySpecialNumber(sensors_event_t a,sensors_event_t g,sensors_event_t temp){
  return ( 50 + 
    ( fabs( (g.gyro.x * a.acceleration.x) + (g.gyro.y * a.acceleration.y) + (g.gyro.z * a.acceleration.z) )/3.41 ) 
  )*(is_turned_on ? 1:0);
}

float readDistanceCM() {
  // Then wait until the ECHO pin goes high, and count the time it stays high (pulse length). 

  // Da se zapocne novo merenje, TRIG pin(koji je output tipa) se setuje na high. 
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10); //Merimo 10 mikrosekundi(ili vise)
  digitalWrite(TRIG_PIN, LOW); //Setujemo isti pin na LOW da bismo zavrsili merenje

  //Nakon toga procitamo vrednost sa ECHO_PIN-a kada je na HIGH
  int duration = pulseIn(ECHO_PIN, HIGH);

  // Duzina ECHO HIGH signala je proporcionalna distanci. Da bismo dobili rezultat u santimetrima, delimo
  // duration sa 58.
  return duration / 58; // vraca distancu u centimetrima
}
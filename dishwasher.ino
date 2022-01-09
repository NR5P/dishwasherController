#include <LiquidCrystal.h>
#include  <Wire.h>
#include <Button.h>

/*
TODO:
1. [done] fix minutes remaining under 10 minutes wash and dry
2. make sure soap dispenser works
3. add pause button
4. add stop button
5. making sure the diverter is working
6. add protection if heater on to long then shut off
8. [done] add rinse after wash
*/

// Creates an LCD object. Parameters: (rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(22, 24, 26, 28, 30, 32);

//Thermistor configuration
// resistance at 25 degrees C
//#define THERMISTORNOMINAL 50000     
// temp. for nominal resistance (almost always 25 C)
//#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 2000
// the value of the 'other' resistor
#define SERIESRESISTOR 50000    
 
int samples[NUMSAMPLES];

//buttin pins
#define startButtonPin 8
Button startButton(startButtonPin);
#define stopButtonPin 9
Button stopButton(stopButtonPin);
#define pauseButtonPin 12
Button pauseButton(stopButtonPin);

// water divert to top and bottom sprayers
#define divertSensorPin 10
Button divertSensor(divertSensorPin);

//Output pins
#define ventPin 7
#define soapDispensor 6
#define waterInlet 5
#define drainPin 4
#define washMotor 3
#define heaterPin 2
#define divertMotorPin 11
//#define tempSensor A0

//Timings
unsigned long fillTime = 95000; //Fill time
unsigned long dryTime = 900000; //Dry time
unsigned long mainWashCycleTime = 3300000; //Main wash cycle time
unsigned long drainTime = 120000; //Drain time
unsigned long rinseTime = 300000; //Rinse time
unsigned long dispenserMotorOnTime = 45000;  //Dispenser motor ON time

bool stopNow = false;

void setup() {
  lcd.begin(16,2);
  lcd.clear();
  
  pinMode(ventPin, OUTPUT);
  pinMode(soapDispensor, OUTPUT);
  pinMode(waterInlet, OUTPUT);
  pinMode(drainPin, OUTPUT);
  pinMode(washMotor, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  pinMode(divertMotorPin, OUTPUT);
  
  digitalWrite(ventPin, LOW);
  digitalWrite(soapDispensor, HIGH);
  digitalWrite(waterInlet, HIGH);
  digitalWrite(drainPin, HIGH);
  digitalWrite(washMotor, HIGH);
  digitalWrite(heaterPin, HIGH);
  digitalWrite(divertMotorPin, HIGH);

  startButton.begin();
  stopButton.begin();
  pauseButton.begin();
  divertSensor.begin();
}

void loop() {
  if (startButton.pressed()) 
  {
    lcd.clear();
    delay(100);
    lcd.setCursor(0,0);
    lcd.print("Start in 5 sec..."); //Start and 5 sec delay
    delay(5000);
    
    lcd.clear();
    fill();
    delay(100);                     // Fill

    lcd.clear();
    rinse();
    delay(100);                     // Rinse

    lcd.clear();
    drain();
    delay(100);                     // Drain

    lcd.clear();
    fill();
    delay(100);                     // Fill again

    lcd.clear();
    wash();
    delay(100);                     //Main wash

    lcd.clear();
    drain();
    delay(100);                     //Drain

    lcd.clear();
    fill();
    delay(100);                     // Fill

    lcd.clear();
    rinse();
    delay(100);                     // Rinse

    lcd.clear();
    drain();
    delay(100);                     // Drain

    lcd.clear();
    dry();
    delay(100);                     //Dry

    lcd.clear();
    while(true){
      actualizarLCD(6, 0);
      digitalWrite(ventPin, LOW);
      digitalWrite(soapDispensor, HIGH);
      digitalWrite(waterInlet, HIGH);
      digitalWrite(drainPin, HIGH);
      digitalWrite(washMotor, HIGH);
      digitalWrite(heaterPin, HIGH);      //Informs cycle complete and waits for power off
    }
  } 
}

void fill() { //Fill cycle

  unsigned long beginningFill = millis(); 
  unsigned long remaining = fillTime; 
  unsigned long actualMillis = 0;      
  unsigned long previoMillis = 0;
 
  while (((millis() - beginningFill) < fillTime) && stopNow == false) { 
    stopNowCheck();    
    digitalWrite(waterInlet, LOW);
    
    actualMillis = millis();
    if((actualMillis - previoMillis) >= 60000){
      remaining -= 60000;
      previoMillis = actualMillis;
    }
    actualizarLCD(1, remaining);   
  }
  digitalWrite(waterInlet, HIGH);
}

void rinse() {
  bool diverted = false;
  unsigned long beginningRinse = millis();
  unsigned long remaining = rinseTime;
  unsigned long divertTime = rinseTime / 2;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while (((millis() - beginningRinse) < rinseTime) && stopNow == false) { 
    stopNowCheck();
    digitalWrite(washMotor, LOW);

    actualMillis = millis();
    if((actualMillis - previoMillis) >= 60000){
      remaining -= 60000;
      previoMillis = actualMillis;
    }
    actualizarLCD(2, remaining);

    if (diverted == false && remaining <= divertTime) {
      diverted = true;
      divert();
    }
 }
 digitalWrite(washMotor, HIGH);
}

void divert() {
  bool divertComplete = false; 
  bool divertStarted = false;
  digitalWrite(washMotor, HIGH);
  delay(1000);
  digitalWrite(divertMotorPin, LOW);
  while (!divertComplete) {
    if (divertStarted && !divertSensor.pressed()) {
      divertComplete = true;
    }
    if (divertSensor.pressed())
      divertStarted = true;
  }

  digitalWrite(divertMotorPin, HIGH);
  delay(1000);
  digitalWrite(washMotor, LOW);
}

void drain() {
  unsigned long beginningDrain = millis();
  unsigned long remaining = drainTime;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while ((millis() - beginningDrain) < drainTime) {    
    
    digitalWrite(drainPin, LOW);
    
    actualMillis = millis();
    
    if((actualMillis - previoMillis) >= 60000){
      remaining -= 60000;
      previoMillis = actualMillis; 
  }
    actualizarLCD(3, remaining);
  }  
  digitalWrite(drainPin, HIGH);
}

void wash() {
  digitalWrite(ventPin, HIGH);
  bool diverted = false;
  unsigned long beginningWash = millis();
  unsigned long remaining = mainWashCycleTime;
  unsigned long divertTime = mainWashCycleTime / 2;
  //double temperature = temp();
  bool dispense = false;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  unsigned long beginningDispense = 0;
  
  while (((millis() - beginningWash) < mainWashCycleTime) && stopNow == false) { 
    stopNowCheck(); 
    //temperature = temp();
    
    digitalWrite(washMotor, LOW);
    digitalWrite(heaterPin, LOW);
    
    actualMillis = millis();
    
    if((actualMillis - previoMillis) >= 60000){
      remaining -= 60000;
      previoMillis = actualMillis; 
    }

    // dispense soap right away, this dishwasher doesn't have temp sensor
    digitalWrite(soapDispensor, LOW);
    dispense = true;
    beginningDispense = millis();
    
    if (diverted == false && remaining <= divertTime) {
      diverted = true;
      divert();
    }
  
  /*
    if((temp() >= 45) && (dispense == false)){    
      digitalWrite(soapDispensor, LOW);
      dispense = true;
      beginningDispense = millis();
    }
*/

    if((millis() - beginningDispense) >= dispenserMotorOnTime){
      digitalWrite(soapDispensor, HIGH);
    }
            
    actualizarLCD(4, remaining);
    
  }
  digitalWrite(washMotor, HIGH);
  digitalWrite(heaterPin, HIGH);
  digitalWrite(soapDispensor, HIGH);
}

void dry() {
  unsigned long inicioSecado = millis();
  unsigned long remaining = dryTime;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while (((millis() - inicioSecado) < dryTime) && stopNow == false) {    
    stopNowCheck(); 
    digitalWrite(heaterPin, LOW);
    
    actualMillis = millis();
    
    if((actualMillis - previoMillis) >= 60000){
      remaining -= 60000;
      previoMillis = actualMillis; 
  }
    if(remaining <= 420000){
      digitalWrite(ventPin, HIGH);
      }
    actualizarLCD(5, remaining);
  }  
  digitalWrite(heaterPin, HIGH);
  digitalWrite(ventPin, LOW);
}

/*
double temp(){
  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(tempSensor);
   delay(10);
  }
  
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  return steinhart;
}
*/

void actualizarLCD(int mode, unsigned long remaining){
  lcd.clear();
  lcd.setCursor(0,0);
  switch (mode){
    case 1:
    lcd.print("Filling");
    break;
    case 2:
    lcd.print("Rinse");
    break;
    case 3:
    lcd.print("Drain");
    break;
    case 4:
    lcd.print("Washing");
    lcd.setCursor(9,0);
    //lcd.print(temp());
    //lcd.print(" C");
    break;
    case 5:
    lcd.print("Drying");
    break;
    case 6:
    lcd.print("Finished! :D");
    break;
    }
    
  lcd.setCursor(0,1);
  lcd.print("Min. Remain. ");
  lcd.print(remaining/60000);
}

void stopNowCheck() {
  if (stopButton.pressed()) {
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("STOP!!!");
    stopNow = true;
  }
}

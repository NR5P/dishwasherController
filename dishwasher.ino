#include <LiquidCrystal.h>
#include  <Wire.h>
#include <Button.h>

// Creates an LCD object. Parameters: (rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(22, 24, 26, 28, 30, 32);

//buttin pins
#define startButtonPin 8
Button startButton(startButtonPin);
#define pauseButtonPin 12
Button pauseButton(pauseButtonPin);

// water divert to top and bottom sprayers
#define divertSensorPin 10
Button divertSensor(divertSensorPin);

// interrupt for stop
#define stopButtonPin 21

//Output pins
#define ventPin 7
#define soapDispensor 6
#define waterInlet 5
#define drainPin 4
#define washMotor 3
#define heaterPin 2
#define divertMotorPin 11

//Timings
unsigned long fillTime = 95000; //Fill time
unsigned long dryTime = 900000; //Dry time
unsigned long mainWashCycleTime = 3300000; //Main wash cycle time
unsigned long drainTime = 120000; //Drain time
unsigned long rinseTime = 300000; //Rinse time
unsigned long dispenserMotorOnTime = 45000;  //Dispenser motor ON time

volatile bool stopNow = false;

void setup() {
  lcd.begin(16,2);
  lcd.clear();

  pinMode(stopButtonPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(stopButtonPin), stopRightNow, CHANGE);
  
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
  pauseButton.begin();
  divertSensor.begin();
}

void loop() {
  if (startButton.pressed()) 
  {
    divert(false, false);
    lcd.clear();
    delay(100);
    lcd.setCursor(0,0);
    lcd.print("Start in 5 sec..."); //Start and 5 sec delay
    delay(5000);
    
    lcd.clear();
    fill();
    delay(100);                     // Fill

    lcd.clear();
    rinse(false);
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
    rinse(false);
    delay(100);                     // Rinse

    lcd.clear();
    drain();
    delay(100);                     // Drain

    lcd.clear();
    fill();
    delay(100);                     // Fill

    lcd.clear();
    rinse(true);
    delay(100);                     // Rinse

    lcd.clear();
    drain();
    delay(100);                     // Drain

    lcd.clear();
    dry();
    delay(100);                     //Dry

    lcd.clear();
    actualizarLCD(6, 0);
    digitalWrite(ventPin, LOW);
    digitalWrite(soapDispensor, HIGH);
    digitalWrite(waterInlet, HIGH);
    digitalWrite(drainPin, HIGH);
    digitalWrite(washMotor, HIGH);
    digitalWrite(heaterPin, HIGH);      //Informs cycle complete and waits for power off
  } 
}

void fill() { //Fill cycle

  unsigned long beginningFill = millis(); 
  unsigned long remaining = fillTime; 
  unsigned long actualMillis = 0;      
  unsigned long previoMillis = 0;
 
  while (((millis() - beginningFill) < fillTime) && stopNow == false) { 
    if (!stopNow)
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

void rinse(bool lastRinseCycle) {
  bool diverted = false;
  unsigned long beginningRinse = millis();
  unsigned long remaining = rinseTime;
  unsigned long divertTime = rinseTime / 2;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while (((millis() - beginningRinse) < rinseTime) && stopNow == false) { 
    if (!stopNow)
      digitalWrite(washMotor, LOW);
    if (lastRinseCycle) {
      digitalWrite(soapDispensor, LOW);
      delay(1000);
      digitalWrite(soapDispensor, HIGH);
    }

    actualMillis = millis();
    if((actualMillis - previoMillis) >= 60000){
      remaining -= 60000;
      previoMillis = actualMillis;
    }
    actualizarLCD(2, remaining);

    if (diverted == false && remaining <= divertTime) {
      diverted = true;
      divert(false, true);
    }
 }
 digitalWrite(washMotor, HIGH);
}

void divert(bool heaterOn, bool washOn) {
  bool divertComplete = false; 
  bool divertStarted = false;
  digitalWrite(washMotor, HIGH);
  if (heaterOn)
    digitalWrite(heaterPin, HIGH);

  delay(1000);
  digitalWrite(divertMotorPin, LOW);
  delay(500);
  while (!divertComplete) {
    if (divertStarted && divertSensor.pressed()) {
      divertComplete = true;
    }
    if (divertSensor.released())
      divertStarted = true;
  }

  digitalWrite(divertMotorPin, HIGH);
  delay(1000);
  if (washOn)
    digitalWrite(washMotor, LOW);
  if (heaterOn)
    digitalWrite(heaterPin, LOW);
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
    //pauseWash(remaining);
    //temperature = temp();
    if (!stopNow) {
      digitalWrite(washMotor, LOW);
      digitalWrite(heaterPin, LOW);
    } 
    
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
      divert(true, true);
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
  digitalWrite(ventPin, LOW);
  unsigned long inicioSecado = millis();
  unsigned long remaining = dryTime;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while (((millis() - inicioSecado) < dryTime) && stopNow == false) {    
    if (!stopNow)
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

void actualizarLCD(int mode, unsigned long remaining){
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
    break;
    case 5:
    lcd.print("Drying");
    break;
    case 6:
    lcd.print("Finished! :D");
    break;
    case 7:
    lcd.print("PAUSED WASH");
    break;
    }
    
  lcd.setCursor(0,1);
  lcd.print("Min. Remain. ");
  lcd.print(remaining/60000);
  lcd.print(" ");
}

void stopRightNow() {
  digitalWrite(ventPin, LOW);
  digitalWrite(soapDispensor, HIGH);
  digitalWrite(waterInlet, HIGH);
  digitalWrite(drainPin, HIGH);
  digitalWrite(washMotor, HIGH);
  digitalWrite(heaterPin, HIGH);
  digitalWrite(divertMotorPin, HIGH);

  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("STOP!!!");
  delay(1000);
  stopNow = true;
}

void pauseWash(unsigned long remaining) {
  bool paused = false;
  if (pauseButton.pressed()) {
    paused = true;
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("PAUSE!!!");
    delay(1000);

    while (paused && !stopNow) {
      actualizarLCD(7, remaining);
      digitalWrite(washMotor, HIGH);
      digitalWrite(heaterPin, HIGH);

      if (pauseButton.pressed()) { 
        paused = false;
      }
    }
  }

  if (!stopNow)
  {
    digitalWrite(washMotor, LOW);
    digitalWrite(heaterPin, LOW);
  }
}

#include  <Wire.h>
#include  <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

//Thermistor configuration
// resistance at 25 degrees C
#define THERMISTORNOMINAL 50000     
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 2000
// the value of the 'other' resistor
#define SERIESRESISTOR 50000    
 
int samples[NUMSAMPLES];

//Output pins
#define ventPin 7
#define soapDispensor 6
#define waterInlet 5
#define drainPin 4
#define washMotor 3
#define heaterPin 2
#define tempSensor A0

//Timings
unsigned long tiempoLlenado = 95000; //Fill time
unsigned long tiempoSecado = 900000; //Dry time
unsigned long tiempoLavado = 3300000; //Main wash cycle time
unsigned long tiempoDrenaje = 120000; //Drain time
unsigned long tiempoEnjuage = 300000; //Rinse time
unsigned long tiempoDispensadoJabon = 45000;  //Dispenser motor ON time

void setup() {
  lcd.init();                     
  lcd.backlight();
  
  pinMode(ventPin, OUTPUT);
  pinMode(soapDispensor, OUTPUT);
  pinMode(waterInlet, OUTPUT);
  pinMode(drainPin, OUTPUT);
  pinMode(washMotor, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  
  digitalWrite(ventPin, LOW);
  digitalWrite(soapDispensor, HIGH);
  digitalWrite(waterInlet, HIGH);
  digitalWrite(drainPin, HIGH);
  digitalWrite(washMotor, HIGH);
  digitalWrite(heaterPin, HIGH);
}

void loop() {
  
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Ciclo en 5 seg..."); //Start and 5 sec delay
  delay(5000);
  
  lcd.clear();
  llenar();
  delay(100);                     // Fill

  lcd.clear();
  enjuagar();
  delay(100);                     // Rinse

  lcd.clear();
  drenar();
  delay(100);                     // Drain

  lcd.clear();
  llenar();
  delay(100);                     // Fill again

  lcd.clear();
  lavar();
  delay(100);                     //Main wash

  lcd.clear();
  drenar();
  delay(100);                     //Drain

  lcd.clear();
  secar();
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

void llenar() { //Fill cycle

  unsigned long inicioLlenado = millis(); 
  unsigned long restante = tiempoLlenado; 
  unsigned long actualMillis = 0;      
  unsigned long previoMillis = 0;
 
  while ((millis() - inicioLlenado) < tiempoLlenado) { 
    
    digitalWrite(waterInlet, LOW);
    
    actualMillis = millis();
    if((actualMillis - previoMillis) >= 60000){
      restante -= 60000;
      previoMillis = actualMillis;
    
  }
    actualizarLCD(1, restante);   
  }
  digitalWrite(waterInlet, HIGH);
}

void enjuagar() {

  unsigned long inicioEnjuage = millis();
  unsigned long restante = tiempoEnjuage;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while ((millis() - inicioEnjuage) < tiempoEnjuage) { 
    digitalWrite(washMotor, LOW);

    actualMillis = millis();
    if((actualMillis - previoMillis) >= 60000){
      restante -= 60000;
      previoMillis = actualMillis;
  }
  actualizarLCD(2, restante);
 }
 digitalWrite(washMotor, HIGH);
}

void drenar() {

  unsigned long inicioDrenaje = millis();
  unsigned long restante = tiempoDrenaje;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while ((millis() - inicioDrenaje) < tiempoDrenaje) {    
    
    digitalWrite(drainPin, LOW);
    
    actualMillis = millis();
    
    if((actualMillis - previoMillis) >= 60000){
      restante -= 60000;
      previoMillis = actualMillis; 
  }
    actualizarLCD(3, restante);
  }  
  digitalWrite(drainPin, HIGH);
}

void lavar() {
  unsigned long inicioLavado = millis();
  unsigned long restante = tiempoLavado;
  double temperatura = temp();
  bool dispensado = false;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  unsigned long inicioDispensado = 0;
  
  while ((millis() - inicioLavado) < tiempoLavado) { 
    
    temperatura = temp();
    
    digitalWrite(washMotor, LOW);
    digitalWrite(heaterPin, LOW);
    
    actualMillis = millis();
    
    if((actualMillis - previoMillis) >= 60000){
      restante -= 60000;
      previoMillis = actualMillis; 
  }
  
    if((temp() >= 45) && (dispensado == false)){    
        digitalWrite(soapDispensor, LOW);
        dispensado = true;
        inicioDispensado = millis();
        }

    if((millis() - inicioDispensado) >= tiempoDispensadoJabon){
        digitalWrite(soapDispensor, HIGH);
      }
            
    actualizarLCD(4, restante);
    
  }
  digitalWrite(washMotor, HIGH);
  digitalWrite(heaterPin, HIGH);
  digitalWrite(soapDispensor, HIGH);
}

void secar() {
  unsigned long inicioSecado = millis();
  unsigned long restante = tiempoSecado;
  unsigned long actualMillis = 0;
  unsigned long previoMillis = 0;
  
  while ((millis() - inicioSecado) < tiempoSecado) {    
    
    digitalWrite(heaterPin, LOW);
    
    actualMillis = millis();
    
    if((actualMillis - previoMillis) >= 60000){
      restante -= 60000;
      previoMillis = actualMillis; 
  }
    if(restante <= 420000){
      digitalWrite(ventPin, HIGH);
      }
    actualizarLCD(5, restante);
  }  
  digitalWrite(heaterPin, HIGH);
  digitalWrite(ventPin, LOW);
}

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

void actualizarLCD(int modo, unsigned long restante){
  
  lcd.setCursor(0,0);
  switch (modo){
    case 1:
    lcd.print("Llenando");
    break;
    case 2:
    lcd.print("Enjuage");
    break;
    case 3:
    lcd.print("Drenaje");
    break;
    case 4:
    lcd.print("Lavando");
    lcd.setCursor(9,0);
    lcd.print(temp());
    lcd.print(" C");
    break;
    case 5:
    lcd.print("Secando");
    break;
    case 6:
    lcd.print("Terminado! :D");
    break;
    }
    
  lcd.setCursor(0,1);
  lcd.print("Min. Rest. ");
  lcd.print(restante/60000);
  }

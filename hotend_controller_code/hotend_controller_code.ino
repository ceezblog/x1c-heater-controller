// HOTEND CONTROLLER V1.0
// Target MCU: ATMega328P (arduino nano clone)
// Clock: Ext 16MHz
//
// Simon - 21/11/2024


// BOARD CONFIGURATION
#define THERMISTOR A0
#define HEATER A3

#define LED 13 //onboard led

// Thermistor reading
#define SERIES_RESISTOR 100000
#define VCC 4950
#define NTC_NOMINAL 100000      
#define NTC_TEMP_NOMINAL 25   
#define B_COEFF 3950
#define NUMSAMPLES 8

#define HEATER_RATE 10  // 10*50ms
#define LED_BLINK_RATE 5 // 5*50ms

#define TARGET_TEMPERATURE 220

// special delay function
#define DELAY_CYCLES(n) __builtin_avr_delay_cycles(n)

float read_NTC()
{
  float Vo=0;
  //read n times
  for (int i=0; i< NUMSAMPLES; i++) 
    Vo += analogRead(THERMISTOR);
    
  Vo /= NUMSAMPLES; //averaging
  
  // convert to resistance R_NTC
  Vo = SERIES_RESISTOR /(1023/Vo  - 1);     // 10K * (ADC - 1) / 1023
    
  float NTC_temp;

  // T_NTC = 1/ [ ln(R_NTC/Ro) / B + 1/(To) ]  deg Kelvin
  // 1/T_NTC = ln(R_NTC /Ro) / B + 1/To, where T_NTC and To in Kelvin
 
  NTC_temp = Vo / NTC_NOMINAL;                    // (R/Ro)
  NTC_temp = log(NTC_temp);                       // ln(R/Ro)
  NTC_temp /= B_COEFF;                            // 1/B * ln(R/Ro)
  NTC_temp += 1.0 / (NTC_TEMP_NOMINAL + 273.15);  // + (1/To)
  NTC_temp = 1.0 / NTC_temp;                      // Invert
  NTC_temp -= 273.15;                             // convert to C

  return NTC_temp;
}

void initTimer()
{
  //initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR2A = 0;
  TCCR2B = 0;
  
  TCNT2 = 1310;            
  TCCR2B = TCCR2B & B11111000 | B00000111;
  TIMSK2 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}


uint16_t HeaterTick;
uint16_t LEDTick;

bool bHeaterUpdate;
bool bLEDUpdate;

ISR(TIMER2_OVF_vect)  // Call every 50ms
{
  TCNT2 = 1310;
  HeaterTick++;
  LEDTick++;

  if (HeaterTick > HEATER_RATE) //update every 10 ticks
  {
    HeaterTick =0;
    bHeaterUpdate = true;
  }

  if (LEDTick > LED_BLINK_RATE) //update every 5 ticks
  {
    LEDTick =0;
    bLEDUpdate = true;
  }
}

/////////////////////////////////////////////////////////////////////
void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);

  initTimer();

  digitalWrite(HEATER, LOW);
  pinMode(HEATER, OUTPUT);
  pinMode(LED, OUTPUT);
  bHeaterUpdate = true;
}

bool led_state;
float nozzle_temp;


// Do not use any blocking function such as delay
void loop() 
{
  if (bHeaterUpdate)
  {
    bHeaterUpdate = 0;
    nozzle_temp = read_NTC();
    if (nozzle_temp >= TARGET_TEMPERATURE)  // turn off heater if reach target temp
      digitalWrite(HEATER, LOW);
    else  // turn on heater
      digitalWrite(HEATER, HIGH);
  }
  
  if (bLEDUpdate) // just blink LED
  {
    bLEDUpdate = 0;
    led_state = !led_state;
    digitalWrite(LED, led_state);
  }
}



#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "PinChangeInt.h"             //interrupt kezeléshez

//lábkiosztás definiálása
#define enc_pend1 4                   //szögelfordulás mérése
#define enc_pend2 5
#define enc_cart1 2                   //kocsi elmozdulásmérése
#define enc_cart2 3
#define rotR 12                       //motor forgásiránya
#define rotL 13     
#define pwm 10                        //PWM jel
#define integral_null 8               //integrátor tag nullázása
#define poti 5                        //poti
#define MAXPWM 230                    //maximális kiadható PWM
#define calibn 7                      //szögelfordulás nullpontjának 
#define calibp 11                     //kalibrálása
#define xnull 9

volatile int lastEncoded_pend = 0;
volatile int lastEncoded_cart = 0;

volatile long sum_pend = -1200;       //nullpont beállítása alsó 
long last_sum_pend = 0;               //egyensúlyi helyzetben
volatile long sum_cart = 0;       //kalibrálás a kocsi szélső, 
                                      //nullpont a középső helyzetében
double last_sum_cart = 0;
double xIntegral = 0;
long lastencoderValue_pend = 0;
long lastencoderValue_cart = 0;

int currPos = 0;
int lastPos = 0;
unsigned long currTime = 0;
unsigned long currTimeM = 0;
unsigned long lastTime = 0;
double sampleTime = 5;                 //mintavételezési idő
double velocity = 0;
double angVelocity = 0;

//szabalyozo parameterei
double p_fi = 90; 
double d_fi = 4.7 / (sampleTime);  
double p_x = 0.005;
double d_x = 0.08 / (sampleTime);

//pwm
int pwmValue;

//haladási irányu
bool R = 0;
int potiVal = 0;

//mozgoatlag
const int M = 1;
double readings[M];
int readIndex = 0;
double total = 0;
double average = 0;



void setup() {
  Serial.begin (9600);

  pinMode(enc_pend1, INPUT_PULLUP); 
  pinMode(enc_pend2, INPUT_PULLUP);
  pinMode(enc_cart1,INPUT);
  pinMode(enc_cart2, INPUT);
  pinMode(integral_null, INPUT);
  pinMode(poti,INPUT);
  pinMode(calibp,INPUT);
  pinMode(calibn,INPUT);
  pinMode(xnull, INPUT_PULLUP);

  //felhúzó ellenállások aktiválása
  digitalWrite(enc_pend1, HIGH);
  digitalWrite(enc_pend2, HIGH);
  digitalWrite(enc_cart1, HIGH);
  digitalWrite(enc_cart2, HIGH);
  digitalWrite(xnull, HIGH);
  
  //updateEncoder() függvény meghívása a fototranzisztor jelszintjének
  //változásakor
  attachPinChangeInterrupt(enc_pend1, updateEncoderPend, CHANGE); 
  attachPinChangeInterrupt(enc_pend2, updateEncoderPend, CHANGE);
  attachPinChangeInterrupt(xnull, xNull, RISING);
  attachInterrupt(0, updateEncoderCart, CHANGE); 
  attachInterrupt(1, updateEncoderCart, CHANGE);
  
  //motorvezérlőhöz tartozó pinek kalibrálása
  pinMode(pwm,OUTPUT);
  pinMode(rotR,OUTPUT);
  pinMode(rotL,OUTPUT);

  pwmValue = 0;
  Left();
  analogWrite(pwm,pwmValue);
//  light = analogRead(LIGHT);
//  last_light = light;
}

void loop(){ 
  currTime = micros();
  //beavatkozó feszültség számítása:
  if(currTime - lastTime >= sampleTime * 1000){
//     currTimeM = micros();
//     movingAverage(sum_cart);
//     if(p_x * average < -5) average = -5 / p_x;
//     if(p_x * average > 5) average = 5 / p_x;     
     pwmValue = p_fi * (double(sum_pend) + p_x * double(sum_cart) + d_x * double(sum_cart - last_sum_cart)) + 
                d_fi * (double(sum_pend - last_sum_pend) + p_x * double(sum_cart) + d_x * double(sum_cart - last_sum_cart));
     last_sum_pend = sum_pend;
     last_sum_cart = sum_cart;
     lastTime = currTime;
     if(pwmValue >= 0) Right();
     else Left();
     pwmValue = abs(pwmValue);
     //ha túl közel vagyunk a szerkezet széléhez:
     if(abs(sum_cart) >= 3200) pwmValue = 0;     
     //telítődés esetére:
     if(pwmValue > MAXPWM) pwmValue = MAXPWM;     
     analogWrite(pwm,pwmValue);
//     if(++i > 50){
//      Serial.println(pwmValue);
//      i=0;
//     }
//     i = micros();
//     Serial.println(i-currTimeM);
//     Serial.print('\t');

//     if(abs(analogRead(LIGHT)-light) > 150)
//     {
//      enabledata = !enabledata;
//      light = analogRead(LIGHT);
//     }
//     enabledata = (abs(analogRead(LIGHT)-light) > 150);  
//     if(enabledata && (i < arraysize)){
//      cartarray[i] = sum_cart;
//      pendarray[i] = sum_pend;
//      i++;
//     }
//     if(!enabledata && i>200){
//      analogWrite(pwm,0);
//      for(int j = 0; j <= i; j++){
//        Serial.print(cartarray[j]);
//        Serial.print('\t');
//        Serial.println(pendarray[j]);
//      }
//      i = 0;
//     }
    }
//
//  //szögelfordulás nullpontjának menet közbeni kalibrálására
  if(digitalRead(calibp)){
    sum_pend = sum_pend + 1;
    delay(300);
  }
  if(digitalRead(calibn)){
    sum_pend = sum_pend - 1;
    delay(300);
  }

  //integrátor menet közbeni nullázására
  if(digitalRead(integral_null)){
    xIntegral = 0;
    sum_cart = 0;
  }
//  //Serial.print(sum_pend);
//  //Serial.print('\t');
//  //Serial.println(digitalRead(calibn));
}

void updateEncoderPend(){
  int MSB = digitalRead(enc_pend1); //MSB = most significant bit
  int LSB = digitalRead(enc_pend2); //LSB = least significant bit
  //a pinek értékének konvertálása egész számmá:
  int encoded = (MSB << 1) |LSB; 
  //az előző értékhez való hozzáadás
  int sum  = (lastEncoded_pend << 2) | encoded; 

  //menetirány megállapítása, ez alapján az elfordulásérték frissítése
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) 
    sum_pend ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) 
    sum_pend --;

  lastEncoded_pend = encoded;
}

void updateEncoderCart(){
  int MSB = digitalRead(enc_cart1);
  int LSB = digitalRead(enc_cart2);

  int encoded = (MSB << 1) |LSB; 
  int sum  = (lastEncoded_cart << 2) | encoded;
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) 
    sum_cart --;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) 
    sum_cart ++;

  lastEncoded_cart = encoded;
}

//ha negatív x irányba kell a kocsit vezérelni
void Left()
{
  digitalWrite(rotR,LOW);
  digitalWrite(rotL,HIGH);
  R = 0;
}

//ha pozitív x irányba kell a kocsit vezérelni
void Right(){
  digitalWrite(rotL,LOW);
  digitalWrite(rotR,HIGH);
  R = 1;
}

void movingAverage(double unit){
  total -= readings[readIndex];
  readings[readIndex] = unit;
  total += readings[readIndex];
  readIndex++;
  if(readIndex >= M) readIndex = 0;
  average = total / M;
}

void xNull(){
  sum_cart = 0;
}

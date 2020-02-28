#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "PinChangeInt.h"             //interrupt kezeléshez

//lábkiosztás definiálása
#define enc_pend1 10                   //szögelfordulás mérése
#define enc_pend2 9
#define enc_cart1 2                   //kocsi elmozdulásmérése
#define enc_cart2 3

volatile int lastEncoded_pend = 0;
volatile int lastEncoded_cart = 0;

volatile long sum_pend = -1200;       //nullpont beállítása alsó 
long last_sum_pend = 0;               //egyensúlyi helyzetben
volatile long sum_cart = 0;       //kalibrálás a kocsi szélső, 



void setup() {
  Serial.begin (115200);

  pinMode(enc_pend1, INPUT_PULLUP); 
  pinMode(enc_pend2, INPUT_PULLUP);
  pinMode(enc_cart1,INPUT);
  pinMode(enc_cart2, INPUT);
  
  //felhúzó ellenállások aktiválása
  digitalWrite(enc_pend1, HIGH);
  digitalWrite(enc_pend2, HIGH);
  digitalWrite(enc_cart1, HIGH);
  digitalWrite(enc_cart2, HIGH);
  
  //updateEncoder() függvény meghívása a fototranzisztor jelszintjének
  //változásakor
  attachPinChangeInterrupt(enc_pend1, updateEncoderPend, CHANGE); 
  attachPinChangeInterrupt(enc_pend2, updateEncoderPend, CHANGE);
  attachInterrupt(0, updateEncoderCart, CHANGE); 
  attachInterrupt(1, updateEncoderCart, CHANGE);

}

void loop(){   
  Serial.print('b');
  Serial.print(sum_cart);
  Serial.print('b');
  Serial.println(sum_pend);
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

#include <BasicLinearAlgebra.h>

using namespace BLA;


BLA::Matrix<3, 3> Ac = {  1.7970, -0.9152, 0.4780, 1, 0, 0, 0, 0.2500, 0};
BLA::Matrix<3, 1> Bc = { 8, 0, 0};
BLA::Matrix<1, 3> Cc = { -2.1776, 3.6057, -5.6883};
BLA::Matrix<1> Dc = {22.4515};

BLA::Matrix<3, 1> X_prec = { 0, 0, 0 };  //condizioni iniziali
BLA::Matrix<1> e_prec = { 0 };
BLA::Matrix<3, 1> Xc;
BLA::Matrix<1> e;
BLA::Matrix<1> u;
double Input = 0;
double misura = 0;
float oldtime = 0;
double k = 0.38;
void setup() {
  // put your setup code here, to run once:
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(5, HIGH);
  digitalWrite(6, LOW);
  Serial.begin(115200);
}

void loop() {
  oldtime = millis();
  Xc = Ac * X_prec + Bc * e_prec;
  misura = map(analogRead(A0), 0, 1023, -135, 135);
  if(misura >-8 && misura <4){ misura =0;}
  e = -misura;  //calcolo errore di approssimazione
  u = Cc * Xc + Dc * e;

  if (misura > -45 && misura < -8) {
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    if (u(0) * k < 200) {
      analogWrite(11, abs(u(0) * k));
    } else {
        analogWrite(11, abs(200));}
  } else if (misura > 4 && misura < 45) {
    digitalWrite(6, HIGH);
    digitalWrite(5, LOW);
    if (u(0) * k < 200) {
      analogWrite(11, abs(u(0) * k));
    } else {
        analogWrite(11, abs(200));}
  } else {
    digitalWrite(6, LOW);
    digitalWrite(5, LOW); }
  Serial.print("misura : ");
  Serial.print(misura);
  Serial.print("\t");
  Serial.print("Output : ");
  Serial.print(u(0)*k);
  Serial.print("\t");
  delayMicroseconds(7500);
  Serial.print("Loop time: ");
  Serial.println(millis()-oldtime);
  
}

#include <BasicLinearAlgebra.h>

using namespace BLA;


BLA::Matrix<4, 4> Ac = {  1.1894, -0.2820, -0.3839, 0, 0.5000, 0, 0, 0, 0, 0.2500, 0, 0, 0, 0, 0.2500, 0};
BLA::Matrix<4, 1> Bc = { 256, 0, 0, 0 };
BLA::Matrix<1, 4> Cc = { -12.0883, 45.7545, -108.6854, 89.5101 };
BLA::Matrix<1> Dc = { 0.0014 };

BLA::Matrix<1> Ah = {1};
BLA::Matrix<1> Bh = {0.1250};
BLA::Matrix<1> Ch = {0.1600};
BLA::Matrix<1> Dh = {0.0100};


BLA::Matrix<4, 1> X_prec = { 1, 1, 1, 1 };  //condizioni iniziali
BLA::Matrix<1> e_prec = { 0 };
BLA::Matrix<4, 1> Xc;
BLA::Matrix<1> e;
BLA::Matrix<1> u;

BLA::Matrix<1> Xh_prec = {0};  //condizioni iniziali
BLA::Matrix<1> yh_prec = {0};
BLA::Matrix<1> Xh;
BLA::Matrix<1> eh;
BLA::Matrix<1> uh;


double Input = 0;
double misura = 0;
float oldtime = 0;
double k = 0.0008;
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
  Serial.print("Loop time: ");
  Serial.println(millis()-oldtime);
  oldtime = millis();

  Xc = Ac * X_prec + Bc * e_prec;
  misura = map(analogRead(A0), 0, 1023, -135, 135);
  if(misura >-8 && misura <4){ misura =0;}
  e = misura;  //calcolo errore di approssimazione
  u = Cc * Xc + Dc * e;
  X_prec  = Xc;
  e_prec = e;


  if (misura > -45 && misura < -8) {
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    if (u(0) * k < 200) {
      analogWrite(11, abs(u(0) * k));
    } else {
        analogWrite(11, abs(200));
    }
  } else if (misura > 4 && misura < 45) {
    digitalWrite(6, HIGH);
    digitalWrite(5, LOW);
    if (u(0) * k < 200) {
      analogWrite(11, abs(u(0) * k));
    } else {
        analogWrite(11, abs(200));
    }
  } else {
    digitalWrite(6, LOW);
    digitalWrite(5, LOW);
  }

  Serial.print("misura : ");
  Serial.print(misura);
  Serial.print("\t");
  Serial.print("Output : ");
  Serial.print(u(0)*k);
  Serial.println("\t");
  
  delayMicroseconds(5000);
  
}

#include <BasicLinearAlgebra.h>

using namespace BLA;


BLA::Matrix<3, 3> A = { 1.6263, -0.6421, 0.1264, 1.0000, 0, 0, 0, 0.1250, 0 };
BLA::Matrix<3, 1> B = { 32, 0, 0 };
BLA::Matrix<1, 3> C = { -2.3459, 3.8848, -12.2823 };
BLA::Matrix<1> D = { 74.3328 };
BLA::Matrix<3, 1> X_prec = { 0, 0, 0 };  //condizioni iniziali
BLA::Matrix<1> e_prec = { 0 };
BLA::Matrix<3, 1> Xc;
BLA::Matrix<1> e;
BLA::Matrix<1> u;
double Input = 0;
float oldtime = 0;
double k = 0.18;
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

  Xc = A * X_prec + B * e_prec;
  Input = map(analogRead(A0), 0, 1023, 45, 315);
  e = 180 - Input;  //calcolo errore di approssimazione
  u = C * Xc + D * e;


  if (Input > 140 && Input < 172) {
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    if (u(0) * k < 200) {
      analogWrite(11, abs(u(0) * k));
    } else {
        analogWrite(11, abs(200));
    }
  } else if (Input > 184 && Input < 220) {
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

  Serial.print("Input : ");
  Serial.print(Input);
  Serial.print("\t");
  Serial.print("Output : ");
  Serial.print(u(0) * k);
  Serial.print("\t");
  Serial.print("Loop time: ");
  Serial.println((millis() - oldtime));
}

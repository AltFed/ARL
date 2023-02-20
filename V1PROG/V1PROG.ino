#include <BasicLinearAlgebra.h>

using namespace BLA;


BLA::Matrix<3, 3> Ac = { 1.6263, -0.6421, 0.1264, 1.0000, 0, 0, 0, 0.1250, 0 };
BLA::Matrix<3, 1> Bc = { 32, 0, 0 };
BLA::Matrix<1, 3> Cc = { -2.3459, 3.8848, -12.2823 };
BLA::Matrix<1> Dc = { 74.3328 };

BLA::Matrix<3, 3> Ah = { 1.6263, -0.6421, 0.1264, 1.0000, 0, 0, 0, 0.1250, 0 };
BLA::Matrix<3, 1> Bh = { 32, 0, 0 };
BLA::Matrix<1, 3> Ch = { -2.3459, 3.8848, -12.2823 };
BLA::Matrix<1> Dh = { 74.3328 };


BLA::Matrix<3, 1> X_prec = { 0, 0, 0 };  //condizioni iniziali
BLA::Matrix<1> e_prec = { 0 };
BLA::Matrix<3, 1> Xc;
BLA::Matrix<1> e;
BLA::Matrix<1> u;

BLA::Matrix<3, 1> Xh_prec = { 0, 0, 0 };  //condizioni iniziali
BLA::Matrix<1> eh_prec = { 0 };
BLA::Matrix<3, 1> Xh;
BLA::Matrix<1> eh;
BLA::Matrix<1> uh;


double Input = 0;
double misura = 0;
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

  Xc = Ac * X_prec + Bc * e_prec;
  misura = map(analogRead(A0), 0, 1023, -135, 135);
  Xh = Ah * Xh_prec + Bh * eh_prec;
  uh = Ch * Xh + Dh *misura;
  e = uh;  //calcolo errore di approssimazione
  u = Cc * Xc + Dc * e;


  if (Input > -45 && Input < -8) {
    digitalWrite(5, HIGH);
    digitalWrite(6, LOW);
    if (u(0) * k < 200) {
      analogWrite(11, abs(u(0) * k));
    } else {
        analogWrite(11, abs(200));
    }
  } else if (Input > 4 && Input < 45) {
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

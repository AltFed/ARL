#include<PID_v1.h>
double Setpoint, Input, Output;
double kp = 100, ki = 0, kd =20;
PID myPID(&Input, &Output, &Setpoint, kp, ki, kd, 1);

int PPot = A0;
int pot = 0;
int IN1 = 5;
int IN2 = 6;
int ENA = 11;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(PPot, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  Input = analogRead(PPot);
  Setpoint = 0; //pendolo sta dritto
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(-90,90);

}

void loop() {
  // put your main code here, to run repeatedly:
  pot = map(analogRead(PPot),0,1023,-145,145);
  Input = pot;
  myPID.Compute();
  Serial.print(Output);
  Serial.print(" , ");
  Serial.print(pot);
  Serial.println();


  if(-40 < pot && pot < 0)    // SI MUOVE A SINISTRA
  {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA,abs(Output));
  //  Serial.println(Output);
  }
  if(0 < pot && pot < 50){    // SI MUOVE A DESTRA
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA,abs(Output));
  //  Serial.println(Output);
  }
  if( pot < -50 ||  pot > 50 ){
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
  if(pot < 5 && pot > -5)
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
}

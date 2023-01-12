#include<PID_v1.h>
double Input,Output,Setpoint;
int minAngle = 2
double kp= 15,ki = 0,kd = 3;
PID myPID(&Input, &Output, &Setpoint,kp,ki,kd,DIRECT);
void setup() {
  // put your setup code here, to run once:
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(11,OUTPUT);
  pinMode(A0,INPUT);
  digitalWrite(5, HIGH);
  digitalWrite(6,LOW);
  Input=0;
  Setpoint=0;
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(-95, 95);
  Serial.begin(9600);
  }

void loop() {
  // put your main code here, to run repeatedly:2
  
  Input=map(analogRead(A0),0,1023,-140,140);  
  myPID.Compute();
  if(Input>minAngle && Input<60){
    digitalWrite(5,LOW);
    digitalWrite(6,HIGH);
    analogWrite(11,abs(Output));
  }else if (Input<-minAnglea && Input>-60){
    digitalWrite(6,LOW);
    digitalWrite(5,HIGH);
    analogWrite(11,abs(Output));
  }else{
  digitalWrite(6,LOW);
  digitalWrite(5,LOW);
  }
  Serial.print(Input);
  Serial.print(" , ");
  Serial.print(Output)
}  

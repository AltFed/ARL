#include<PID_v1.h>
double Input,Output,Setpoint;
int minAngle = 0;
double kp= 3,ki = 0.01,kd = 1;
PID myPID(&Input, &Output, &Setpoint,kp,ki,kd,REVERSE);


const int numReadings = 8;

int readings[numReadings];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
int average = 0;    



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
  myPID.SetOutputLimits(-120, 120);
  Serial.begin(9600);

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  }

void loop() {
  // put your main code here, to run repeatedly:2
  

    // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(A0);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  // calculate the average:
  average = total / numReadings;

  // send it to the computer as ASCII digit

  Input=map(average,0,1023,-145,145);  
  myPID.SetSampleTime (60);
  myPID.Compute();

  if(Input>minAngle && Input<60){
    digitalWrite(5,LOW);
    digitalWrite(6,HIGH);
    analogWrite(11,abs(Output));
  }else if (Input<-minAngle && Input>-60){
    digitalWrite(6,LOW);
    digitalWrite(5,HIGH);
    analogWrite(11,abs(Output));
  }else{
  digitalWrite(6,LOW);
  digitalWrite(5,LOW);
  }
  
  Serial.print(map(analogRead(A0),0,1023,-145,145));
  Serial.print("\t");
  Serial.print(Output);
  Serial.print("\t");
  
  Serial.println();
}  

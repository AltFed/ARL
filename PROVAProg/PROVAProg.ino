double A[][]={{1.6263,   -0.6421 ,   0.1264},{1.0000         0         0},{0    0.1250         0}};
double B[][]={{ 32},{0},{0}};
double C[][]={{-2.3459  ,  3.8848 , -12.2823}};
double D[][]={74.3328};
int X_prec[3]={{1,1,1}};
int e_prec[1]={0};
int Xc[3];
int e[3];
float u;
void setup() {
  // put your setup code here, to run once:
 pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(11,OUTPUT);
  pinMode(A0,INPUT);
  digitalWrite(5, HIGH);
  digitalWrite(6,LOW);
  Serial.begin(9600);
}

void loop() {
  double A_temp[][]=A;
  double B_temp[][]=B;
  double C_temp[][]=C;
  double D_temp[][]=D;
  double K[][];
  double H[][];
  double W[][];
  double Q[][];
  double R[][];
  double I[][];
  // put your main code here, to run repeatedly:
   for (int i = 0; i<3; ++i)  
        for (int j = 0; j<3; ++j)  
            for (int k = 0; k<3; ++k)  
            {  
                K[i][j] += A_temp[i][k] * X_prec[j];  
            }  
             for (int i = 0; i<3; ++i)  //rig B
               for (int j = 0; j<3; ++j)  //col e
                for (int k = 0; k<1; ++k)  //col B
                {  
                    H[i][j] += B_temp[i][k] * e_prec[j];  
               }  
             for (int i = 0; i < 3; i++){  
              for (int j = 0; j < 3; j++){  
                 R[i][j] = K[i][j] + M[i][j];  
    Xc = R;
    for(int i=0;i<3;i++){
      e[0][0]=180-analogRead(A0);
    }
                  for (int i = 0; i<1; ++i)  //rig c
               for (int j = 0; j<3; ++j)  //col Xc
                for (int k = 0; k<3; ++k)  //col C
                {  
                    W[i][j] += C_temp[i][k] * Xc[k][j];  
               } 
              for (int i = 0; i<1; ++i)  //rig D
               for (int j = 0; j<3; ++j)  //col e
                for (int k = 0; k<1; ++k)  //col D
                {  
                    Q[i][j] += D_temp[i][k] * e[k][j];  
               }  
               for (int i = 0; i < 1; i++){  
                for (int j = 0; j < 3; j++){  
                  I[i][j] = W[i][j] + Q[i][j];                 
    u = (I[0][0]+I[0][];


}
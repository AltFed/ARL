double A[][]={{1.6263,   -0.6421 ,   0.1264},{1.0000         0         0},{0    0.1250         0}};
double B[][]={{ 32},{0},{0}};
double C[][]={{-2.3459  ,  3.8848 , -12.2823}};
double D[][]={74.3328};
double X_prec[3][1]={{0,0,0}};//condizioni iniziali
double e_prec[1]={0};
double Xc[3][1];
double e[1];
double u;
double Input=0;
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
  double A_temp[3][3]=A;
  double B_temp[3][1]=B;
  double C_temp[1][3]=C;
  double D_temp[1][1]=D;
  double K[3][1];
  double H[3][1];
  double W[1][1];
  double Q[1][1];
  double R[3][1];
  double I[1][1];
  // put your main code here, to run repeatedly:
   for (int i = 0; i<3; ++i){  //rig A
        for (int j = 0; j<1; ++j){  //col X
            for (int k = 0; k<3; ++k)  //col A
            {  
                K[i][j] += A_temp[i][k] * X_prec[k][j];  //->dim 3x1
            }  
        }
   }
             for (int i = 0; i<3; ++i){  //rig B
               for (int j = 0; j<1; ++j){  //col e
                for (int k = 0; k<1; ++k)  //col B
                {  
                    H[i][j] += B_temp[i][k] * e_prec[k];  //->dim 3x1
               }  
               }
             }

             for (int i = 0; i < 3; i++){  
              for (int j = 0; j < 1; j++){  
                 R[i][j] = K[i][j] + M[i][j];  //-> dim 3x1
              }
             }
    Xc = R; //dim 3x1
    Input=map(analogRead(A0),0,1023,-145,145)
    e=180-Input;  //calcolo errore di approssimazione 
                  for (int i = 0; i<1; ++i){  //rig C
               for (int j = 0; j<1; ++j){ //col Xc
                for (int k = 0; k<3; ++k)  //col C
                {  
                    W[i][j] += C_temp[i][k] * Xc[k][j];  //->dim 1x1
               } 
               }
                  }
              for (int i = 0; i<1; ++i){  //rig D
               for (int j = 0; j<1; ++j){  //col e
                for (int k = 0; k<1; ++k)  //col D
                {  
                    Q[i][j] += D_temp[i][k] * e[k];  //-> dim 1x1
               }  
               }
              }
               for (int i = 0; i < 1; i++){  
                for (int j = 0; j < 1; j++){  
                  I[i][j] = W[i][j] + Q[i][j];     // sono scalari è una matrice 1x1     
                }
               }       
    u = I;// scalare 
     Serial.print(u);
     Serial.print("\t");
     if(Input> -80 &&  Input <-12){
    digitalWrite(5,HIGH);
    digitalWrite(6,LOW);
    analogWrite(11,abs(u));
  }else if (Input >6 && Input <80){
    digitalWrite(6,HIGH);
    digitalWrite(5,LOW);
    analogWrite(11,abs(u));
  }else{
  digitalWrite(6,LOW);
  digitalWrite(5,LOW);
  }    
}
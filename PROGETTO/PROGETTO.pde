float lw=30,l1=50,l2=200,l3=100,l4=100,l5=50,l6=30;  //link
float g = 50;
float xd=200,yd=300,zd=0;
float q[] = {0,0,0,0,0,0};
float xBase, yBase;
float eyeY,segno = 1;
float alfa=0,beta=0,theta=0;//pinza orientata verso il basso
float[][] Re = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R36 = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R03 = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R03T = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T03 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T36 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T06 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float pwx,pwy,pwz;
float A1,A2;


void setup(){
  size(1000,800,P3D);
  strokeWeight(2);
  stroke(0);
  xBase=width/2;
  yBase=height/2;
}


void draw()
{
  background(50);
  lights();
  camera((width/2.0), height/2 - eyeY, (height/2.0) / tan(PI*60.0 / 360.0), width/2.0, height/2.0, 0, 0, 1, 0);
  if(mousePressed){
    xBase = mouseX;
    yBase = mouseY;
  }
  if (keyPressed)
  {
    // movimento camera
    if (keyCode == DOWN)
    {
      eyeY -= 5;
    }
    if (keyCode == UP)
    {
      eyeY += 5;
    }
    // movimento alfa,beta,theta(pinza)
    if(key == 'a'){
      alfa -= .01;
    }
    if(key == 'A'){
      alfa += .01;
    }
    if(key == 'b'){
      beta -= .01;
    }
    if(key == 'B'){
      beta += .01;
    }
    if(key == 't'){
      theta -= .01;
    }
    if(key == 'T'){
      theta += .01;
    }
    // gomito alto-gomito basso
    if(key == '+'){
      segno = 1;
    }
    if(key == '-'){
      segno = -1;
    }
    // movimento punto desiderato
    if(key == 'x'){
      xd -= 5;
    }
    if(key == 'X'){
      xd += 5;
    }
    if(key == 'y'){
      yd -= 5;
    }
    if(key == 'Y'){
      yd += 5;
    }
    if(key == 'z'){
      zd -= 5;
    }
    if(key == 'Z'){
      zd += 5;
    }
  }
  pushMatrix();
     initRe();
   // funzione per il movimento 
    muovi();
    initR03();
   //  funzione per definire il manipolatore
    robot(); 
  // funzione per la grafica
   popMatrix();
   graphic();
}


void initRe(){
  Re[0][0]=(-cos(alfa)*cos(beta)*cos(theta)-sin(alfa)*sin(theta));
  Re[0][1]=(-cos(alfa)*sin(beta)*sin(theta)+sin(alfa)*cos(theta));
  Re[0][2]=(cos(alfa)*cos(beta));
  Re[1][0]=(sin(alfa)*sin(beta)*cos(theta)-sin(theta)*cos(alfa));
  Re[1][1]=(sin(alfa)*sin(beta)*sin(theta)+cos(alfa)*cos(theta));
  Re[1][2]=(-sin(alfa)*sin(beta));
  Re[2][0]=(-cos(theta)*cos(beta));
  Re[2][1]=(cos(beta)*sin(theta));
  Re[2][2]=(-sin(beta));
}


void initR03(){
  R03[0][0]=(cos(q[0])*cos(q[1]+q[2]));
  R03[0][1]=(-cos(q[0])*sin(q[1]+q[2]));
  R03[0][2]=(-sin(q[0]));
  R03[1][0]=(sin(q[0])*cos(q[1]+q[2]));
  R03[1][1]=(-sin(q[0])*sin(q[1]+q[2]));
  R03[1][2]=(cos(q[0]));
  R03[2][0]=(-sin(q[1]+q[2]));
  R03[2][1]=(-cos(q[1]+q[2]));
  R03[2][2]=0;
}


void robot(){
// creo la sfera che indica il punto desiderato
 translate(xd,yd,zd);
 ellipse(0,0,10,10);
 translate(-xd,-yd,-zd);
// centro base manipolatore
  translate(xBase, yBase, 0);
//--------link 0--------
  box(l1,l1,l1);           
  fill(255,0,0);
//link 1
  rotateY(q[0]);   
  translate(0,-l1,0);
  box(l1,l1,l1);
  
//struttura per il link 2
  translate(l1/2-lw/2,-l1/2-lw/2,0);
  rotateZ(q[1]);  
  box(lw,lw,g);     
  
//link 2
  translate(l2/2+lw/2,0,0); // TRANSLATE ORIGINALE
  box(l2,lw,lw);          
  
//struttura link 3
  translate(l2/2-lw/2,0,0);
  rotateZ(q[2]);
  box(lw,lw,g);          

//link3  
  translate(0,-l3/2-lw/2,0);
  
  box(lw,l3,lw); 
  
//link 4
  translate(0,-l4,0);
  rotateY(q[3]);
  box(lw,l4,lw);
  
//struttura link 5
  translate(0,-l4/2-lw/2,0);
  rotateY(q[4]);
  box(lw,lw,g);
  
//link 5
  translate(l5/2+lw/2,0,0);
  box(l5,lw,lw);
  
//link 6
  translate(l5/2+lw/2,0,0);
  rotateX(q[5]);
  box(l6,lw,lw);
}


void muovi(){
  //cinematica inversa 
 pwx=(xd-xBase)-((l5+l6)*Re[0][2]);
 pwy=(-yd+yBase)-((l5+l6)*Re[1][2]);
 pwz=zd-(l5+l6)*Re[2][2];  
 q[0]=atan2(pwy,pwx);
 A1=pwx*cos(q[0])+pwy*sin(q[0])-l1;
 A2=(l1+l1)-pwz;
 q[2]=asin((pow(A1,2) +pow(A2,2)-pow(l2,2)-pow(l3+l4,2))/(A2*l2*(l3+l4)));
 q[1]=atan2((l3+l4)*cos(q[2])*A1-(l2+(l3+l4)*sin(q[2]))*A2,l2+(l3+l4)*sin(q[2])*A1+(l3+l4)*cos(q[2])*A2);
 R03T=trasposta(R03);
 R36=mProd(R03T,Re);
 q[4]=acos(R36[2][2]);// ho preso il segno positivo
 if(sin(q[4])>0){
   q[3]=atan2(R36[1][2],R36[0][2]);
   q[5]=atan2(R36[2][1],-R36[2][0]);
 }
 if(sin(q[4])<0){
  q[3]=atan2(-R36[1][2],-R36[0][2]);
  q[5]=atan2(-R36[2][1],R36[2][0]);
  }
}

void graphic(){
  textSize(20);
  fill(255);

  text("xd = ",10,20);
  text(xd-xBase,50,20);//coordinate rispetto alla base
  
  text("yd = ",10,40);
  text(-yd+yBase,50,40);//coordinate rispetto alla base
  
  text("zd = ",10,60);
  text(zd,50,60);
  

  text("alfa = ",120,20);
  text((alfa*180)/PI,173,20);
  
  text("beta = ",120,40);
  text((beta*180)/PI,180,40);
  
  text("theta = ",120,60);
  text((theta*180)/PI,190,60);
  
  
  if(segno==1){
    text("gomito alto ",10,80);
  }else if ( segno == -1){
    text("gomito basso",10,80);
  }
  
  scriviMatrice("Re = ",Re,10,100);
  text("theta 1 = ", 300, 120);
  text(sin(q[0]), 400, 120);
  text(q[0]*180/PI, 480, 120);
  text("theta 2 = ", 300, 120+100);
  text(sin(q[1]), 400, 120+100);
  text(q[1]*180/PI, 480, 120+100);
  text("theta 3 = ", 300, 120+200);
  text(sin(q[2]), 400, 120+200);
  text(q[2]*180/PI, 480, 120+200);
  text("theta 4 = ", 300, 120+300);
  text(sin(q[3]), 400, 120+300);
  text(q[3]*180/PI, 480, 120+300);
  text("theta 5 = ", 300, 120+400);
  text(sin(q[4]), 400, 120+400);
  text(q[4]*180/PI, 480, 120+400);
  text("theta 6 = ", 300, 120+500);
  text(sin(q[5]), 400, 120+500);
  text(q[5]*180/PI, 480, 120+500);
  //scrivo matrice di traslazione T03
  T03[0][0]=(cos(q[0])*cos(q[1]+q[2]));
  T03[0][1]=sin(q[0]);
  T03[0][2]=(cos(q[0])*sin(q[1]+q[2]));
  T03[0][3]=(l1*cos(q[0])+l2*cos(q[0])*cos(q[1]));
  T03[1][0]=(sin(q[0])*cos(q[1]+q[2]));
  T03[1][1]=-cos(q[0]);
  T03[1][2]=(sin(q[0])*sin(q[1]+q[2]));
  T03[1][3]=(l1*sin(q[0])+l2*sin(q[0]+q[1]));
  T03[2][0]=0;
  T03[2][1]=0;
  T03[2][2]=0;
  T03[2][3]=1;
  // scrivo T36
  T36[0][0]=(cos(q[3])*cos(q[4])*cos(q[5])-sin(q[3])*sin(q[5]));
  T36[0][1]=(-cos(q[3])*cos(q[4])*cos(q[5])-sin(q[3])*cos(q[5]));
  T36[0][2]=(cos(q[3])*sin(q[4]));
  T36[0][3]=((l5+l6)*cos(q[3])*sin(q[4]));
  T36[1][0]=(sin(q[3])*cos(q[4])*cos(q[5])+cos(q[3])*sin(q[5]));
  T36[1][1]=(-sin(q[3])*cos(q[4])*sin(q[5])+cos(q[3])*cos(q[5]));
  T36[1][2]=(sin(q[3])*sin(q[4]));
  T36[1][3]=((l5+l6)*sin(q[3])*sin(q[4]));
  T36[2][0]=(-sin(q[4])*cos(q[5]));
  T36[2][1]=(sin(q[4])*sin(q[5]));
  T36[2][2]=cos(q[4]);
  T36[2][3]=((l5+l6)*cos(q[4])+(l3+l4));
  T36[3][0]=0;
  T36[3][1]=0;
  T36[3][2]=0;
  T36[3][3]=1;
  //voglio calcolare T06 e poi ottenere xd yd zd (cinematica diretta)
  T06=mProd(T03,T36);
  text("xd effettivo = ",700,120);
  text(T06[0][3],800,120);
  text("yd effettivo = ",700,120+70);
  text(T06[1][3],800,120+70);
  text("zd effettivo = ",700,120+120);
  text(T06[2][3],800,120+120);
  
}

// ----- FUNZIONI PER MATRICI -----


// ----- CALCOLO PRODOTTO FRA MATRICI ------
float[][] mProd(float[][] A,float[][] B) // Calcola prodotto di due matrici A e B
{
  int nA = A.length;
  int nAB = A[0].length;
  int nB = B[0].length;
  
  float[][] C = new float[nA][nB]; 

  for (int i=0; i < nA; i++) 
  {
    for (int j=0; j < nB; j++) 
    {  
      for (int k=0; k < nAB; k++) 
      {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
  return C;
}


// ----- CALCOLO TRASPOSTA -----
float[][] trasposta(float[][] A) // Calcola la trasposta di una matrice A
{
  int nR = A.length;
  int nC = A[0].length; 
  
  float[][] C = new float[nC][nR]; 

  for (int i=0; i < nC; i++) 
  {
    for (int j=0; j < nR; j++) 
    {  
      C[i][j] = A[j][i];
    }
  }
  return C;
}


// ----- STAMPARE UNA MATRICE ------
void scriviMatrice(String s, float[][] M, int x, int y) // Scrive una matrice a partire dal punto (x,y)
{
  textSize(20);
  fill(255);
  text(s,x,y); 
  text(M[0][0],x,y+30); text(M[0][1],x+90,y+30); text(M[0][2],x+180,y+30);
  text(M[1][0],x,y+60); text(M[1][1],x+90,y+60); text(M[1][2],x+180,y+60); 
  text(M[2][0],x,y+90); text(M[2][1],x+90,y+90); text(M[2][2],x+180,y+90);
}  

float lw=40,l1=20,l2=40,l3=5,l4=5,l5=5,l6=5;  //link
float g = 50;
float q[] = {0,0,0,0,0,0};
float q_eff[]={0,0,0,0,0,0};
float xBase, yBase,zBase,xDes,yDes,zDes;
float x6,y6,z6;
float xd=60,yd=30,zd=40,a;
float eyeY,segno = 1;
float alfa=PI/2,beta=PI,theta=PI/2;//pinza orientata verso il basso
float[][] Re = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R03 = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R03T = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R36 = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T03 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T36 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T06 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float pwx,pwy,pwz;
float A1,A2;
float kp=.1,dis=0.0001;
float xf=0,yf=0,zf=0;
float T1,T2,d1,d4,d6;
///         rimettere gli assi della pinza calcolarli nella posizione senza le rotate
void setup(){
  size(1000,800,P3D);
  strokeWeight(2);
  stroke(0);
  xBase=width/2;
  yBase=height/2;
  zBase=0;
  //T1=lw/2;
  //T2=l2+lw;
  //d1=l1+l1+lw/2;
  //d4=lw+l3+l4;
  //d6=lw/2+l5+l6;
  d1=60;
  T1=20;
  T2=80;
  d4=50;
  d6=30;
  
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
      alfa -= .1;
    }
    if(key == 'A'){
      alfa += .1;
    }
    if(key == 'b'){
      beta -= .1;
    }
    if(key == 'B'){
      beta += .1;
    }
    if(key == 't'){
      theta -= .1;
    }
    if(key == 'T'){
      theta += .1;
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
    if(key == 'k'){  
      kp -= 0.001;
    }
    if(key == 'K'){
      kp += 0.001;
  }
  }
  xf=-xBase+xd+width/2;
  yf=-yBase+yd+height/2;
  zf=zBase+zd;
  pushMatrix();
  //inizializzo matrici
   initRe();
   //
   initR03();
// funzione per il movimento 
    muovi();
//  funzione per definire il manipolatore  
   robot();
// funzione per la grafica
   popMatrix();
   graphic();
}


void initRe(){
  
  //Re[0][0]=(cos(alfa)*sin(beta)*cos(theta)-sin(alfa)*sin(theta));
  //Re[0][1]=(-cos(alfa)*sin(beta)*sin(theta)-sin(alfa)*cos(theta));
  //Re[0][2]=(cos(alfa)*cos(beta));
  //Re[1][0]=(sin(alfa)*sin(beta)*cos(theta)+cos(alfa)*sin(theta));
  //Re[1][1]=(-sin(alfa)*sin(beta)*sin(theta)+cos(alfa)*cos(theta));
  //Re[1][2]=(sin(alfa)*cos(beta));
  //Re[2][0]=(-cos(theta)*cos(beta));
  //Re[2][1]=(cos(beta)*sin(theta));
  //Re[2][2]=(sin(beta));
  Re[0][0]=1;
  Re[0][1]=0;
  Re[0][2]=0;
  Re[1][0]=0;
  Re[1][1]=-1;
  Re[1][2]=0;
  Re[2][0]=0;
  Re[2][1]=0;
  Re[2][2]=-1;

}


void initR03(){
  
  R03[0][0]=(cos(q[0])*cos(q[1]+q[2]));
  R03[0][2]=(cos(q[0])*sin(q[1]+q[2]));
  R03[0][1]=(sin(q[0]));
  R03[1][0]=(sin(q[0])*cos(q[1]+q[2]));
  R03[1][2]=(sin(q[0])*sin(q[1]+q[2]));
  R03[1][1]=(-cos(q[0]));
  R03[2][0]=(sin(q[1]+q[2]));
  R03[2][2]=(-cos(q[1]+q[2]));
  R03[2][1]=0;
  
}


void robot(){
  //spostamento angoli del manipolatore
    if (abs(q_eff[0]-q[0])>dis)
    {
      q_eff[0] += kp*(q[0]-q_eff[0]);
    }
    if (abs(q_eff[1]-q[1])>dis)
    {
      q_eff[1] += kp*(q[1]-q_eff[1]);
    }
    if (abs(q_eff[2]-q[2])>dis)
    {
      q_eff[2] += kp*(q[2]-q_eff[2]);
    }
    if (abs(q_eff[3]-q[3])>dis)
    {
      q_eff[3] += kp*(q[3]-q_eff[3]);
    }
    if (abs(q_eff[4]-q[4])>dis)
    {
      q_eff[4] += kp*(q[4]-q_eff[4]);
    }
    if (abs(q_eff[5]-q[5])>dis)
    {
      q_eff[5] += kp*(q[5]-q_eff[5]);
    }
 // translate(width/2+xd,height/2+yd,zd);
 // ellipse(0,0,20,20);
 // translate(-width/2-xd,-height/2-yd,-zd);
 // ASSE Y0
  stroke(0,255,0);
  line(xBase,yBase,zBase, xBase+ 120, yBase,zBase);
  //ASSE Z0
  stroke(0,0,255);
  line(xBase,yBase,zBase, xBase, yBase-120,zBase);
  //ASSE X0
  stroke(255,0,0);
  line(xBase,yBase,zBase, xBase, yBase,zBase+120);
  stroke(0);
  
  translate(xBase, yBase, zBase);
  
  //box==pinza
<<<<<<< HEAD
  //translate(T06[1][3],-T06[2][3],T06[0][3]);
  //box(50,50,50);
  //translate(-T06[1][3],T06[2][3],-T06[0][3]);
////--------link 0--------
  box(l1,l1,l1);           
  fill(255,0,0);
  
//link 1  
  translate(0,-l1,0);
  rotateY(q_eff[0]); 
  box(l1,l1,l1);
  
//struttura per il link 2
  translate(l1/2-lw/2,-l1/2-lw/2,0);
  rotateZ(q_eff[1]);  
  box(lw,lw,g); 
  
//link 2
  translate(l2/2+lw/2,0,0); 
  box(l2,lw,lw);          
  
//struttura link 3
  translate(l2/2+lw/2,0,0); 
  rotateZ(q_eff[2]);
  box(lw,lw,g);          
=======
  translate(T06[1][3],-T06[2][3],T06[0][3]);
  box(20,20,20);
  translate(-T06[1][3],T06[2][3],-T06[0][3]);
  
//LINK 0 -----------------

  box(lw,lw,lw);
  fill(255,0,0);
//------------------------

//LINK 1 -----------------

  translate(0,-lw,0);
  rotateY(-PI/2+q_eff[0]);
  box(l1,l1,l1);
//------------------------

//GIUNTO 2 --------------

  translate(l1/2-lw/2,-l1/2-lw/2,0);
  rotateZ(q_eff[1]);
  box(lw,lw,l1);
//-----------------------

//LINK 2 ----------------

  translate(lw/2+l2/2,0,0);
  box(l2,lw,lw);
//-----------------------

//GIUNTO 3 --------------

  translate(l2/2+lw/2,0,0);
  rotateZ(PI+q_eff[2]);
  box(lw,lw,lw);
//-----------------------

//LINK 3 ----------------
>>>>>>> 8f46472971749a4414d27887c82192840f57b402

  translate(0,l3/2+lw/2,0);
  box(lw,l3,lw);
//-----------------------

//LINK 4 ----------------

  translate(0,l4,0);
  rotateY(q_eff[3]);
  box(lw,l4,lw);
//-----------------------

//GIUNTO 5 --------------

  translate(0,lw/2+l4/2,0);
  rotateZ(q_eff[4]);
  box(lw,lw,lw+15);
//-----------------------

//LINK 5 ----------------
  
  translate(0,lw/2+l5/2,0);
  box(lw,l5,lw);
//------------------------

//LINK 6 ----- PINZA -----

  translate(0,lw/2+l5/2,0);
  rotateX(q_eff[5]);
  box(lw,lw,lw);
  
  
  
  
  
  
  
  //Sistema pinza rispetto la base
  
  //ASSE Y6
  stroke(0,255,0);
  line(0,0,0,200,0,0);
  //ASSE X6
  stroke(255,0,0);
  line(0,0,0,0,200,0);
  //ASSE Z6
  stroke(0,0,255);
  line(0,0,0,0,0,120);
  stroke(0);
}


void muovi(){
 pwx=(xf-((d6)*Re[0][2]));//60
   text(d6*Re[0][2],300,450);
 pwy=(yf-((d6)*Re[1][2]));//40
 pwz=(zf-((d6)*Re[2][2])); //125
 text("val = ",300,400);
 q[0]=atan2(pwy,pwx);//26.57
 A1=pwx*cos(q[0])+pwy*sin(q[0])-T1;//52,08
 A2=(d1)-pwz;//-20
 if(segno==1){
   q[2]=PI-asin((pow(A1,2) +pow(A2,2)-pow(T2,2)-pow(d4,2))/(2*(T2)*(d4)));
 }if(segno==-1){
   q[2]=asin((pow(A1,2) +pow(A2,2)-pow(T2,2)-pow(d4,2))/(2*(T2)*(d4)));
 }
 q[1]=atan2((d4)*cos(q[2])*A1-(T2+(d4)*sin(q[2]))*A2,(T2+(d4)*sin(q[2]))*A1+(d4)*cos(q[2])*A2); 
 R03T=trasposta(R03);
 R36=mProd(R03T,Re);
 q[4]=atan2(sqrt(pow(R36[0][2],2)+pow(R36[1][2],2)),R36[2][2]);
 // ho preso il segno positivo scelta arbitraria
 q[3]=atan2(R36[1][2],R36[0][2]);
 q[5]=atan2(R36[2][1],-R36[2][0]);
 //calcolo x6 y6 z6
  x6=T1*cos(q[0])+(T2)*cos(q[0])*cos(q[1]) + (d4)*cos(q[0])*sin(q[1]+q[2]) + (d6)*(cos(q[0])*(cos(q[1]+q[2])*cos(q[3])*sin(q[4])+ sin(q[1]+q[2])*cos(q[4])) + sin(q[0])*sin(q[3])*sin(q[4]));
  y6=T1*sin(q[0])+(T2)*sin(q[0])*cos(q[1]) + (d4)*sin(q[0])*sin(q[1]+q[2]) + (d6)*(sin(q[0])*(cos(q[1]+q[2])*cos(q[3])*sin(q[4])+ sin(q[1]+q[2])*cos(q[4])) - cos(q[0])*sin(q[3])*sin(q[4]));
  z6=(d1)+(T2)*sin(q[1])-(d4)*cos(q[1]+q[2])+(d6)*(sin(q[1]+q[2])*cos(q[3])*sin(q[4])-cos(q[1]+q[2])*cos(q[4]));
  
  
  //cinematica diretta
////scrivo matrice di traslazione T03
  T03[0][0]=(cos(q[0])*cos(q[1]+q[2]));
  T03[0][1]=sin(q[0]);
  T03[0][2]=(cos(q[0])*sin(q[1]+q[2]));
  T03[0][3]=(T1*cos(q[0])+(T2)*cos(q[0])*cos(q[1]));
  T03[1][0]=(sin(q[0])*cos(q[1]+q[2]));
  T03[1][1]=-cos(q[0]);
  T03[1][2]=(sin(q[0])*sin(q[1]+q[2]));
  T03[1][3]=(T1*sin(q[0])+(T2)*sin(q[0])*cos(q[1]));
  T03[2][0]=sin(q[1]+q[2]);
  T03[2][1]=0;
  T03[2][2]=-cos(q[1]+q[2]);
  T03[2][3]=(d1)+(T2)*sin(q[1]);
  T03[3][0]=0;
  T03[3][1]=0;
  T03[3][2]=0;
  T03[3][3]=1;
   //scrivo T36
  T36[0][0]=(cos(q[3])*cos(q[4])*cos(q[5])-sin(q[3])*sin(q[5]));
  T36[0][1]=(-cos(q[3])*cos(q[4])*sin(q[5])-sin(q[3])*cos(q[5]));
  T36[0][2]=(cos(q[3])*sin(q[4]));
  T36[0][3]=((d6)*cos(q[3])*sin(q[4]));
  T36[1][0]=(sin(q[3])*cos(q[4])*cos(q[5])+cos(q[3])*sin(q[5]));
  T36[1][1]=(-sin(q[3])*cos(q[4])*sin(q[5])+cos(q[3])*cos(q[5]));
  T36[1][2]=(sin(q[3])*sin(q[4]));
  T36[1][3]=((d6)*sin(q[3])*sin(q[4]));
  T36[2][0]=(-sin(q[4])*cos(q[5]));
  T36[2][1]=(sin(q[4])*sin(q[5]));
  T36[2][2]=cos(q[4]);
  T36[2][3]=(((d6))*cos(q[4])+(d4));
  T36[3][0]=0;
  T36[3][1]=0;
  T36[3][2]=0;
  T36[3][3]=1;
  //voglio calcolare T06 e poi ottenere xd yd zd (cinematica diretta)
  T06=mProd(T03,T36);





}

void graphic(){
  textSize(20);
  fill(255);
  text("xf = ",10,20);
  text(xf,50,20);//coordinate rispetto alla base
  
  text("yf = ",10,40);
  text(yf,50,40);//coordinate rispetto alla base
  
  text("zf = ",10,60);
  text(zf,50,60);
  

  text("pwx = ",120,20);
  text(pwx,200,20);
  
  text("pwy = ",120,40);
  text(pwy,400,40);
  
  text("pwz = ",120,60);
  text(pwz,400,60);
  text("A1 = ",500,40);
  text(A1,600,40);
  text("A2 = ",500,60);
  text(A2,600,60);
  text("kp = ",120,80);
  text(kp,180,80);
  
  if(segno==1){
    text("gomito alto ",10,80);
  }else if ( segno == -1){
    text("gomito basso",10,80);
  }
  scriviMatrice("R36 = ",R36,10,400);
  scriviMatriceColor("Re = ",Re,10,100);
  scriviMatrice("R03 =",R03,10,600);
  text("theta 1 = ", 300, 120);
  text(sin(q[0]), 400, 120);
  text(q[0]*180/PI, 480, 120);
  text("theta 2 = ", 300, 120+100);
  text(sin(q[1]), 400, 120+100);
  text(q[1]*180/PI, 480, 120+100);
  text("theta 3 = ", 300, 120+200);
  text(sin(q[2]), 400, 120+200);
  text(q[2]*180/PI, 600, 120+200);
  text("theta 4 = ", 300, 120+300);
  text(sin(q[3]), 400, 120+300);
  text(q[3]*180/PI, 600, 120+300);
  text("theta 5 = ", 300, 120+400);
  text(sin(q[4]), 400, 120+400);
  text(q[4]*180/PI, 480, 120+400);
  text("theta 6 = ", 300, 120+500);
  text(sin(q[5]), 400, 120+500);
  text(q[5]*180/PI, 480, 120+500);
  
  text("xd base = ",700,120);
  text(T06[0][3],850,120);
  text("yd base = ",700,120+70);
  text(T06[1][3],850,120+70);
  text("zd base = ",700,120+120);
  text(T06[2][3],850,120+120);  
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

void scriviMatriceColor(String s, float[][] M, int x, int y) // Scrive una matrice a partire dal punto (x,y)
{
  textSize(20);
  fill(255);
  text(s,x,y);
  fill(255,0,0);
  text(M[0][0],x,y+30);  text(M[1][0],x,y+60);text(M[2][0],x,y+90);
  fill(0,255,0);
  text(M[0][1],x+90,y+30); text(M[1][1],x+90,y+60); text(M[2][1],x+90,y+90); 
  fill(0,0,255);
  text(M[0][2],x+180,y+30); text(M[1][2],x+180,y+60); text(M[2][2],x+180,y+90);
  fill(0);
}  

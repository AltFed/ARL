// FEDERICO ROSI - 0292417 E DANIELE DI GIAMBERARDINO -0291944   
// con x minuscolo si diminuisce la coordinata x in processing mentre diminuisce la yd , con X maiuscolo la si aumenta.
// con y minuscolo si diminuisce la coordinata y in processing mentre diminuisce la zd , con Y maiuscolo la si aumenta.
// con z minuscolo si diminuisce la coordinata z in processing mentre diminuisce la xd , con Z maiuscolo la si aumenta.
// a minuscolo per diminuire α e A maiuscolo per aumentarlo. 
// b minuscolo per diminuire β e B maiuscolo per aumentarlo. 
// t minuscolo per diminuire θ e T maiuscolo per aumentarlo.
// con i tasti + e - si passa dalla soluzione GOMITO ALTO a quella GOMITO BASSO.

float lw=30,l1=50,l2=200,l3=100,l4=100,l5=50,l6=30;  //link
float g = 50;
float q[] = {0,0,0,0,0,0};
float q_eff[]={0,0,0,0,0,0};
float xBase, yBase,zBase,xDes,yDes,zDes;
float x6,y6,z6;
float xd=0,yd=0,zd=0,a;
float eyeY,segno = 1;
float alfa=0,beta=-90,theta=0;//pinza orientata verso il basso
float[][] Re = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R03 = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R03T = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] R36 = new float[3][3]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T03 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T36 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float[][] T06 = new float[4][4]; // matrice 3x3 dichiarata ma non inizializzata
float pwx,pwy,pwz;
float A1,A2;
float kp=.1,dis=.000001;
float xf=0,yf=0,zf=0;
float T1,T2,d1,d4,d6;
float nGiri[] = {0,0,0,0,0,0}; // conta quanti giri su se stesso ha fatto il robot
///         rimettere gli assi della pinza calcolarli nella posizione senza le rotate
void setup(){
  size(1000,800,P3D);
  strokeWeight(2);
  stroke(0);
  xBase=width/2;
  yBase=height/2;
  zBase=0;
  T1=lw/2;
  T2=l2+lw;
  d1=l1+l1+lw/2;
  d4=lw+l3+l4;
  d6=lw/2+l5+l6;
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
// ovvero R06
  Re[0][0]=(cos(alfa)*cos(PI/2 - beta)*cos(theta)-sin(alfa)*sin(theta));
  Re[0][1]=(-cos(alfa)*cos(PI/2 - beta)*sin(theta)-sin(alfa)*cos(theta));
  Re[0][2]=(cos(alfa)*sin(PI/2 - beta));
  Re[1][0]=(sin(alfa)*cos(PI/2 - beta)*cos(theta)+cos(alfa)*sin(theta));
  Re[1][1]=(-sin(alfa)*cos(PI/2 - beta)*sin(theta)+cos(alfa)*cos(theta));
  Re[1][2]=(sin(alfa)*sin(PI/2 - beta));
  Re[2][0]=(-cos(theta)*sin(PI/2 - beta));
  Re[2][1]=(sin(PI/2 - beta)*sin(theta));
  Re[2][2]=(cos(PI/2 - beta));

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
      if (abs(q_eff[0]-q[0]) > abs(q_eff[0]-(q[0] + 2*PI)))
    {
      q[0] = q[0] + 2*PI;
      nGiri[0] += 1;
    }
    if (abs(q_eff[0]-q[0]) > abs(q_eff[0]-(q[0] - 2*PI)))
    {
      q[0] = q[0] - 2*PI;
      nGiri[0] -= 1;
    }
    if (abs(q_eff[0]-q[0])>dis)
    {
      q_eff[0] += kp*(q[0]-q_eff[0]);
    }
    if (abs(q_eff[1]-q[1]) > abs(q_eff[1]-(q[1] + 2*PI)))
    {
      q[1] = q[1] + 2*PI;
      nGiri[1] += 1;
    }
    if (abs(q_eff[1]-q[1]) > abs(q_eff[1]-(q[1] - 2*PI)))
    {
      q[1] = q[1] - 2*PI;
      nGiri[1] -= 1;
    }
    if (abs(q_eff[1]-q[1])>dis)
    {
      q_eff[1] += kp*(q[1]-q_eff[1]);
    }
    if (abs(q_eff[2]-q[2]) > abs(q_eff[2]-(q[2] + 2*PI)))
    {
      q[2] = q[2] + 2*PI;
      nGiri[2] += 1;
    }
    if (abs(q_eff[2]-q[2]) > abs(q_eff[2]-(q[2] - 2*PI)))
    {
      q[2] = q[2] - 2*PI;
      nGiri[2] -= 1;
    }
    if (abs(q_eff[2]-q[2])>dis)
    {
      q_eff[2] += kp*(q[2]-q_eff[2]);
    }
   if (abs(q_eff[3]-q[3]) > abs(q_eff[3]-(q[3] + 2*PI)))
    {
      q[3] = q[3] + 2*PI;
      nGiri[3] += 1;
    }
    if (abs(q_eff[3]-q[3]) > abs(q_eff[3]-(q[3] - 2*PI)))
    {
      q[3] = q[3] - 2*PI;
      nGiri[3] -= 1;
    }
    if (abs(q_eff[3]-q[3])>dis)
    {
      q_eff[3] += kp*(q[3]-q_eff[3]);
    }
       if (abs(q_eff[4]-q[4]) > abs(q_eff[4]-(q[4] + 2*PI)))
    {
      q[4] = q[4] + 2*PI;
      nGiri[4] += 1;
    }
    if (abs(q_eff[4]-q[4]) > abs(q_eff[4]-(q[4] - 2*PI)))
    {
      q[4] = q[4] - 2*PI;
      nGiri[4] -= 1;
    }
    if (abs(q_eff[4]-q[4])>dis)
    {
      q_eff[4] += kp*(q[4]-q_eff[4]);
    }
   if (abs(q_eff[5]-q[5]) > abs(q_eff[5]-(q[5] + 2*PI)))
    {
      q[5] = q[5] + 2*PI;
      nGiri[5] += 1;
    }
    if (abs(q_eff[5]-q[5]) > abs(q_eff[5]-(q[5] - 2*PI)))
    {
      q[5] = q[5] - 2*PI;
      nGiri[5] -= 1;
    }
    if (abs(q_eff[5]-q[5])>dis)
    {
      q_eff[5] += kp*(q[5]-q_eff[5]);
    }
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
  translate(yd,-zd,+xd);
  ellipse(0,0,20,20);
  translate(-yd,+zd,-xd);
//LINK 0 -----------------

  box(lw,lw,lw);
  fill(255,0,0);
//------------------------

//LINK 1 -----------------

  translate(0,-lw,0);
  rotateY(PI+PI/2+q_eff[0]);//-PI/2
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
  rotateZ(-PI+q_eff[2]);//PI
  box(lw,lw,lw);
   //ASSE X3
  stroke(255,0,0);
  line(0,0,0,200,0,0);
  //ASSE Z3
  stroke(0,0,255);
  line(0,0,0,0,-200,0);
  //ASSE Y3
  stroke(0,255,0);
  line(0,0,0,0,0,120);
  stroke(0);
//-----------------------

//LINK 3 ----------------

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
  box(lw,lw,lw);
//-----------------------

//LINK 5 ----------------

  translate(0,lw/2+l5/2,0);
  box(lw,l5,lw);
//------------------------

//LINK 6 ----- PINZA -----

  translate(0,+lw/2+l5/2,0);
  rotateY(q_eff[5]);
  box(lw,lw,lw);







  //Sistema pinza rispetto la base
 //ASSE X6
  stroke(255,0,0);
  line(0,0,0,-200,0,0);
  //ASSE Z6
  stroke(0,0,255);
  line(0,0,0,0,-200,0);
  //ASSE Y6
  stroke(0,255,0);
  line(0,0,0,0,0,200);
  stroke(0);
}

void muovi(){
 pwx=(xd-((d6)*Re[0][2]));//60
 pwy=(yd-((d6)*Re[1][2]));//40
 pwz=(-zd-((d6)*Re[2][2]))+185; //125
 q[0]=atan2(pwy,pwx)+nGiri[0]*2*PI;//26.57
 A1=pwx*cos(q[0])+pwy*sin(q[0])-T1;//52,08
 A2=(d1)-pwz;//-20
 if(segno==1){
   q[2]=PI-asin((pow(A1,2) +pow(A2,2)-pow(T2,2)-pow(d4,2))/(2*(T2)*(d4)))+nGiri[2]*2*PI;
 }if(segno==-1){
   q[2]=asin((pow(A1,2) +pow(A2,2)-pow(T2,2)-pow(d4,2))/(2*(T2)*(d4)))+nGiri[2]*2*PI;
 }
 q[1]=atan2((d4)*cos(q[2])*A1-(T2+(d4)*sin(q[2]))*A2,(T2+(d4)*sin(q[2]))*A1+(d4)*cos(q[2])*A2)+nGiri[1]*2*PI;
 R03T=trasposta(R03);
 R36=mProd(R03T,Re);
 q[4]=atan2(sqrt(pow(abs(R36[0][2]),2)+pow(abs(R36[1][2]),2)),R36[2][2])+nGiri[4]*2*PI;
 // ho preso il segno positivo scelta arbitraria
 q[3]=atan2(R36[1][2],R36[0][2])+nGiri[3]*2*PI;
 q[5]=atan2(R36[2][1],-R36[2][0])+nGiri[5]*2*PI;
 //calcolo x6 y6 z6
  //x6=T1*cos(q[0])+(T2)*cos(q[0])*cos(q[1]) + (d4)*cos(q[0])*sin(q[1]+q[2]) + (d6)*(cos(q[0])*(cos(q[1]+q[2])*cos(q[3])*sin(q[4])+ sin(q[1]+q[2])*cos(q[4])) + sin(q[0])*sin(q[3])*sin(q[4]));
  //y6=T1*sin(q[0])+(T2)*sin(q[0])*cos(q[1]) + (d4)*sin(q[0])*sin(q[1]+q[2]) + (d6)*(sin(q[0])*(cos(q[1]+q[2])*cos(q[3])*sin(q[4])+ sin(q[1]+q[2])*cos(q[4])) - cos(q[0])*sin(q[3])*sin(q[4]));
  //z6=(d1)+(T2)*sin(q[1])-(d4)*cos(q[1]+q[2])+(d6)*(sin(q[1]+q[2])*cos(q[3])*sin(q[4])-cos(q[1]+q[2])*cos(q[4]));


//  //cinematica diretta per debug e controllo dei valori delle soluzioni. 

//////scrivo matrice di traslazione T03
//  T03[0][0]=(cos(q[0])*cos(q[1]+q[2]));
//  T03[0][1]=sin(q[0]);
//  T03[0][2]=(cos(q[0])*sin(q[1]+q[2]));
//  T03[0][3]=(T1*cos(q[0])+(T2)*cos(q[0])*cos(q[1]));
//  T03[1][0]=(sin(q[0])*cos(q[1]+q[2]));
//  T03[1][1]=-cos(q[0]);
//  T03[1][2]=(sin(q[0])*sin(q[1]+q[2]));
//  T03[1][3]=(T1*sin(q[0])+(T2)*sin(q[0])*cos(q[1]));
//  T03[2][0]=sin(q[1]+q[2]);
//  T03[2][1]=0;
//  T03[2][2]=-cos(q[1]+q[2]);
//  T03[2][3]=(d1)+(T2)*sin(q[1]);
//  T03[3][0]=0;
//  T03[3][1]=0;
//  T03[3][2]=0;
//  T03[3][3]=1;
//   //scrivo T36
//  T36[0][0]=(cos(q[3])*cos(q[4])*cos(q[5])-sin(q[3])*sin(q[5]));
//  T36[0][1]=(-cos(q[3])*cos(q[4])*sin(q[5])-sin(q[3])*cos(q[5]));
//  T36[0][2]=(cos(q[3])*sin(q[4]));
//  T36[0][3]=((d6)*cos(q[3])*sin(q[4]));
//  T36[1][0]=(sin(q[3])*cos(q[4])*cos(q[5])+cos(q[3])*sin(q[5]));
//  T36[1][1]=(-sin(q[3])*cos(q[4])*sin(q[5])+cos(q[3])*cos(q[5]));
//  T36[1][2]=(sin(q[3])*sin(q[4]));
//  T36[1][3]=((d6)*sin(q[3])*sin(q[4]));
//  T36[2][0]=(-sin(q[4])*cos(q[5]));
//  T36[2][1]=(sin(q[4])*sin(q[5]));
//  T36[2][2]=cos(q[4]);
//  T36[2][3]=(((d6))*cos(q[4])+(d4));
//  T36[3][0]=0;
//  T36[3][1]=0;
//  T36[3][2]=0;
//  T36[3][3]=1;
//  //voglio calcolare T06 e poi ottenere xd yd zd (cinematica diretta)
//  T06=mProd(T03,T36);
}

void graphic(){
  textSize(20);
  fill(255,0,0);
  //X
  text("xd = ",270,20);
  text(xd,320,20);
  //Y
  fill(0,255,0);

  
  text("yd = ",270,40);
  text(yd,320,40);
  
  //Z
  fill(0,0,255);
  
  text("zd = ",270,60);
  text(zd,320,60);
  
  
  //ALFA
  fill(255,255,255);
  text("alfa = ",400,20);
  text(alfa,500,20);
  //BETA
  text("beta = ",400,40);
  text(beta,500,40);
  //THETA
  text("theta = ",400,60);
  text(theta,500,60);
  
    //ALFA
  fill(255,255,255);
  text("Q4 -Q5",600,20);
  text(q[4] - q[5],700,20);
  //BETA
  text("theta 5 = ",600,40);
  text(q[4],700,40);
  //THETA
  text("theta 6 =",600,60);
  text(q[5],700,60);
  
  fill(255);
  
  text("kp = ",120,80);
  text(kp,180,80);
  
  if(segno==1){
    text("gomito alto ",10,80);
  }else if ( segno == -1){
    text("gomito basso",10,80);
  }
  scriviMatriceColor("Re = ",Re,10,100);
    fill(255);
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

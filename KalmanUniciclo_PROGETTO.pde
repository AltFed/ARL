/*
Questo sketch nasce come evoluzione di 

  unicicloConControlloSuVel.pde

ed effettua un controllo sulla base della stima della
posa dell'uniciclo ottenuta mediante un filtro di Kalman
esteso che fonde l'odometria (movimento ruote) con le 
misure di distanza da un certo numero di landmark.

Quindi, anche qui si disegna un robot mobile di tipo 'uniciclo'
e lo si controlla in velocità (senza limiti di saturazione)
verso il punto cliccato col mouse mediante la legge
di controllo proporzionale, tuttavia il calcolo del controllo
è fatto sulle variabili stimate anziché su quelle vere.
Sulla schermata viene anche disegnato (in giallo) il robot
nella posizione stimata (quello vero è disegnato in rosso).
In più, dei triangoli in bianco con bordo rosso rappresentano 
la posizione dei landmark presenti nell'ambiente. All'interno
di ciascun triangolo viene anche riportato un identificatore del
tipo Li per il landmark i-esimo.

Con le frecce è possibile modificare la frequenza delle
misure di distanza dai landmark o, più precisamente, il
tempo tStep che intercorre tra una misura e la successiva
(che comunque non può essere inferiore al ciclo di Processing,
per default dt = 1/60 secondo): con le frecce SU e GIU si 
incrementa e decrementa rispettivamente di 1 tale tempo mentre 
con le frecce DESTRA e SINISTRA lo si moltiplica e divide 
rispettivamente per 2 (per modificarlo più velocemente). Quando 
tale tempo è molto grande la ricostruzione diviene puramente 
odometrica.

Sulla schermata vengono riportati: le coordinate
(x,y,theta) dell'uniciclo, le coordinate (xDes,yDes)
del punto da raggiungere e il tempo impiegato per la
missione. Inoltre si riportano le velocità angolari 
(omegaR, omegaL) delle due ruote, la velocità longitudinale 
v1 e quella angolare v2.
Infine si indicano anche le grandezze stimate (xHat,yHat,thetaHat)
con la loro matrice di covarianza P e il tempo tStep (modificabile 
da tastiera) che intercorre tra una lettura e la successiva.
*/

// Dimensioni finestra
int sizeX = 1250;//1000;
int sizeY =  950;//550;

// Coordinate attuali uniciclo
float x = 0;
float y = 0;
float theta = 0;

// Coordinate desiderate uniciclo
float xDes = 0;
float yDes = 0;

// Caratteristiche fisiche uniciclo
float r = 8; // raggio ruote in pixel
float d = 25; // distanza tra le ruote in pixel
float w = 5; // spessore ruota in pixel
float R = 1.2*sqrt(pow(r,2)+pow(d/2+w/2,2)); // raggio robot

float dt = (float) 1/60; // tempo di campionamento

float e_p = 0; // errore posizionamento
float v1 = 0; // velocità lineare robot
float kv1 = 1; // costante legge proporzionale controllo v1
float v2 = 0; // velocità rotazionale robot
float kv2 = 2;  // costante legge proporzionale controllo v2
int nGiri = 0; // conta quanti giri su se stesso ha fatto il robot
float thetaDes; // orientamento desiderato (per raggiungere il target)
float t0,tempo; // tempo inizio e attuale missione
float tempoUltimaMisura; // tempo in cui si è effettuata l'ultima misura
long nStime = 0; // numero progressivo totale delle stime fatte

float omegaR = 0; // velocità angolare ruota destra
float omegaL = 0; // velocità angolare ruota sinistra
float uR, uL, uRe, uLe; // spostamenti ruote destra e sinistra veri e presunti (comandati)

// Variabili relative alla stima e al filtro di Kalman esteso
float sigmaX0 = 10; // deviazione standard dell'errore di stima del valore iniziale di x0
float sigmaY0 = 10; // deviazione standard dell'errore di stima del valore iniziale di y0
float sigmaTheta0 = 10*PI/180; // deviazione standard (in rad) dell'errore di stima del valore iniziale di theta0
float xHat = x + Gaussian(0,sigmaX0); // stima di x inizializzata al valore vero + una perturbazione gaussiana a media 0 e deviazione standard sigmaX0
float yHat = y + Gaussian(0,sigmaY0); // stima di y inizializzata al valore vero + una perturbazione gaussiana a media 0 e deviazione standard sigmaY0
float thetaHat = theta + Gaussian(0,sigmaTheta0); // stima di theta inizializzata al valore vero + una perturbazione gaussiana a media 0 e deviazione standard sigmaTheta0 rad
float xHatMeno,yHatMeno,thetaHatMeno; // stime a priori di x,y,theta
float KR = 0.01; // coefficiente varianza errore odometrico ruota destra
float KL = KR; // coefficiente varianza errore odometrico ruota sinistra
float stDevR,stDevL; // deviazione standard errore odometrico ruota destra e sinistra
//float [][] Landmark = {{0,sizeY/2-20}};  // Coordinate landmark (un solo landmark)
//float [][] Landmark = {{0,-sizeY/2+20},{0,sizeY/2-20}}; // Coordinate landmark (due landmark)
float [][] Landmark = {{-sizeX/2+20,-sizeY/2+20},{sizeX/2-20,-sizeY/2+20},{0,sizeY/2-20}};  // Coordinate landmark (3 landmark)
int nL = Landmark.length; // numero totale di landmark
float[] misureLandmark = new float[nL]; // vettore con le misure vere di distanza dagli nL landmark
float[] misureAtteseLandmark = new float[nL]; // vettore con le misure attese di distanza dagli nL landmark
float[] AngoloLandmark = new float[nL];
float[] AngoloLandmarkAtteso = new float[nL]; 
// Seguono le matrici utilizzate dal filtro di Kalman esteso (EKF)
float[][] F = {{1, 0, 0},{0, 1, 0},{0, 0, 1}}; // matrice giacobiana F=df/dx (alcuni elementi delle prime due righe vanno aggiustati durante l'esecuzione)
float[][] P = {{pow(sigmaX0,2), 0, 0},{0, pow(sigmaY0,2), 0},{0, 0, pow(sigmaTheta0,2)}}; // matrice di covarianza P inizializzata in base all'incertezza iniziale
float[][] Pmeno = new float[3][3]; // matrice di covarianza a priori P-
float[][] W = {{1, 0},{0,1},{1/d,-1/d}}; //  matrice giacobiana W = df/dw (gli elementi delle prime due righe vanno aggiustati durante l'esecuzione) 
float[][] Q = {{1, 0},{0,1}}; // matrice di covarianza del rumore di misura odometrico w (gli elementi sulla diagonale vanno aggiustati durante l'esecuzione)
float[][] H = new float[nL][3]; // matrice giacobiana H = dh/dx
float[][] K = new float[3][nL]; // guadagno di Kalman
float DeltaX,DeltaY,DeltaXY; // variabili di supporto
float sigmaLandmark = 5; // deviazione standard errore di misura di distanza dai landmark (in pixel)
float[][] Rs = idMat(nL,pow(sigmaLandmark,2)); // matrice di covarianza errore misura dai landmark
float[][] innovazione = new float[nL][1]; // innovazione EKF
float[][] correzione = new float[3][1]; // termine correttivo stima
float tStep = 0; // tempo (in ms) tra una misura e la successiva (impostabile da tastiera)
float rMax=200,betaMax=PI/10;
float [] num= new float [3];
float [] temp= new float [3];
float [] premutoSU= new float [3];
int nGiriT = 0;
boolean [] ans= new boolean [3];
void setup() 
{
  size(1250,950);
  tempoUltimaMisura = 0; // Inizializzo a zero il tempo in cui è stata effettuata l'ultima misura
  num[0]=1;
  num[1]=1;
  num[2]=1;
}

void draw() 
{
  
  background(0);

  pushMatrix();
  translate(sizeX/2,sizeY/2);
  text("num[0] =",100,200);
  text(num[0],200,200);
  text(str(ans[0]),300,200);
  text("num[1]=",100,300);
  text(num[1],200,300);
  text(str(ans[1]),300,300);
  text("num[2]=",100,400);
  text(num[2],200,400);
  text(str(ans[2]),300,400);
  text("AngoloLandmark[0]=",200,50);
  text((AngoloLandmark[0]*180)/PI,400,50);
  text("AngoloLandmark[1]=",200,100);
  text((AngoloLandmark[1]*180)/PI,400,100);
  text("AngoloLandmark[2]=",200,150);
  text((AngoloLandmark[2]*180)/PI,400,150);
  text((AngoloLandmarkAtteso[0]*180)/PI,500,50);
  text((AngoloLandmarkAtteso[1]*180)/PI,500,100);
  text((AngoloLandmarkAtteso[2]*180)/PI,500,150);
  if (keyPressed)
  {
    if (keyCode == UP) // aumento di 1 il tempo tra una misura e la successiva
    {
      tStep += 1;
    }
    if (keyCode == DOWN)  // decremento di 1 il tempo tra una misura e la successiva
    {
      tStep = max(0,tStep-1);
    }
    if (keyCode == RIGHT) // moltiplico per due il tempo tra una lettura e la successiva
    {
      tStep = tStep*2;
    }
    if (keyCode == LEFT) // divido per due il tempo tra una lettura e la successiva
    {
      tStep = tStep/2;
    }
    if(key == 'b' && betaMax>=((10*PI)/180)){
      betaMax -=.1;
    }
    if(key == 'B' && betaMax<=((140*PI)/180)){
      betaMax +=.1;
    }
    if(key == 'r' && rMax>=51 ){
      rMax -= 50;
    }
    if(key == 'R'){
      rMax += 50;
    }
    if(key == '1'){
        premutoSU[0]=1;
    }
    if(key == '2'){
        premutoSU[1]=1;
    }
    if(key == '3'){
        premutoSU[2]=1; 
    }
    

  
  } else
  {
    if (premutoSU[0] == 1)
    {
      num[0]++;
      premutoSU[0] = 0;
    }
    if(premutoSU[1] == 1){
      num[1]++;
      premutoSU[1] = 0;
    }
    if(premutoSU[2] == 1){
      num[2]++;
      premutoSU[2] = 0;
    }}
    
  if(num[0] % 2 == 0){
    ans[0] = true;
  }else{
    ans[0] = false;
  }
  if(num[1] % 2 == 0){
    ans[1] = true;
  }else{
    ans[1] = false;
  }
  if(num[2] % 2 == 0){
    ans[2]=true;
  }else{
    ans[2]= false;
  }
  
  // Disegno il robot vero e quello stimato
  robot(x,y,theta,1); // l'argomento 1 fa un robot rosso (robot reale)
  robot1(xHat,yHat,thetaHat,0); // l'argomento 0 un robot giallo (robot nella posa stimata)
  
  for (int indLandmark=0; indLandmark<nL;indLandmark++)
  {
    stroke(255,0,0);
    strokeWeight(2);  
    fill(255,255,255);
    triangle(Landmark[indLandmark][0]-15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0]+15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0],-Landmark[indLandmark][1]-15);
    textSize(10);
    fill(0,0,0);
    text("L",Landmark[indLandmark][0]-5,-Landmark[indLandmark][1]+8);
    text(indLandmark+1,Landmark[indLandmark][0]+1,-Landmark[indLandmark][1]+8);
    stroke(0,0,0);
    strokeWeight(1);
  }
  if(ans[0]){
  stroke(255,0,0);
  strokeWeight(2);  
    fill(10,10,10);
    triangle(Landmark[0][0]-15,-Landmark[0][1]+15,Landmark[0][0]+15,-Landmark[0][1]+15,Landmark[0][0],-Landmark[0][1]-15);
    textSize(10);
    fill(0,0,0);
    text("L",Landmark[0][0]-5,-Landmark[0][1]+8);
    text(0+1,Landmark[0][0]+1,-Landmark[0][1]+8);
    noStroke();
    strokeWeight(1);
  }
  if(ans[1]){
    stroke(255,0,0);
    strokeWeight(2);
    fill(10,10,10);
    triangle(Landmark[1][0]-15,-Landmark[1][1]+15,Landmark[1][0]+15,-Landmark[1][1]+15,Landmark[1][0],-Landmark[1][1]-15);
    textSize(10);
    fill(0,0,0);
    text("L",Landmark[1][0]-5,-Landmark[1][1]+8);
    text(1+1,Landmark[1][0]+1,-Landmark[1][1]+8);
    noStroke();
    strokeWeight(1);
  }
  if(ans[2]){
    stroke(255,0,0);
    strokeWeight(2);
    fill(10,10,10);
    triangle(Landmark[2][0]-15,-Landmark[2][1]+15,Landmark[2][0]+15,-Landmark[2][1]+15,Landmark[2][0],-Landmark[2][1]-15);
    textSize(10);
    fill(0,0,0);
    text("L",Landmark[2][0]-5,-Landmark[2][1]+8);
    text(2+1,Landmark[2][0]+1,-Landmark[2][1]+8);
    noStroke();
    strokeWeight(1);
  }
  if (mousePressed) // assegno target
  {
    xDes = mouseX - sizeX/2;
    yDes = sizeY/2 - mouseY;
    t0 = millis(); // inizio conteggio del tempo di missione
  }
// Calcolo errore e controllo basandomi sul valore stimato (xHat,yHat,thetaHat) delle variabili dell'uniciclo
  e_p = sqrt(pow(xDes-xHat,2)+pow(yDes-yHat,2));
  if (e_p > 1) // mi muovo solo se l'errore è maggiore di una certa quantità
  {
    tempo = (millis()-t0)/1000;  // tempo missione in secondi

    // assegno velocità secondo legge proporzionale (in termini delle quantità stimate!)
    v1 = -kv1*((xHat-xDes)*cos(thetaHat) + (yHat-yDes)*sin(thetaHat));

    // Calcolo l'angolo verso il target: scelgo il multiplo di 2PI 
    // più vicino all'orientamento corrente ***stimato*** del robot
    thetaDes = atan2(yDes-yHat,xDes-xHat) + nGiri*2*PI;
    if (abs(thetaDes+2*PI-thetaHat) < abs(thetaDes-thetaHat))
    {
      thetaDes = thetaDes+2*PI;
      nGiri += 1;
    }
    else
    {
      if (abs(thetaDes-2*PI-thetaHat) < abs(thetaDes-thetaHat))
      {
        thetaDes = thetaDes-2*PI;
        nGiri += -1;
      }
    }
    
   // assegno velocità angolare secondo legge proporzionale sempre in funzione delle quantità stimate   
    v2 = kv2*(thetaDes-thetaHat);
  }
  else // se penso di essere vicino al target mi fermo
  {
    v1 = 0;
    v2 = 0;
  }
  
  // Calcolo i movimenti da impartire alle ruote in base alle v1 e v2 trovate
  omegaR = (v1+v2*d/2)/r;
  omegaL = (v1-v2*d/2)/r;
  uRe = r*omegaR*dt; // spostamento comandato alla ruota destra (da considerarsi anche come informazione odometrica disponibile sul suo spostamento in (t,t+dt))
  uLe = r*omegaL*dt; // spostamento comandato alla ruota sinistra (da considerarsi anche come informazione odometrica disponibile sul suo spostamento in (t,t+dt))
  // Perturbo i due movimenti: gli spostamenti reali delle ruote non saranno mai esattamente uguali a quelli comandati 
  stDevR = sqrt(KR*abs(uRe));
  stDevL = sqrt(KL*abs(uLe));  
  uR = uRe + Gaussian(0,stDevR); // Spostamento vero ruota destra
  uL = uLe + Gaussian(0,stDevL); // Spostamento vero ruota sinistra
  
  // Dinamica effettiva dell'uniciclo
  x = x + ((uR+uL)/2)*cos(theta);
  y = y + ((uR+uL)/2)*sin(theta);
  theta = theta + (uR-uL)/d;

  // STIMA FILTRO KALMAN ESTESO: PASSO di PREDIZIONE
  xHatMeno = xHat + ((uRe+uLe)/2)*cos(thetaHat);
  yHatMeno = yHat + ((uRe+uLe)/2)*sin(thetaHat);
  thetaHatMeno = thetaHat + (uRe-uLe)/d;

  //Aggiorno la giacobiana F (solo gli elementi variabili)
  F[0][2] = -(uRe+uLe)*sin(thetaHat)/2;
  F[1][2] = (uRe+uLe)*cos(thetaHat)/2;
  
  // Aggiorno W (solo gli elementi variabili)
  W[0][0] = .5*cos(thetaHat);
  W[0][1] = .5*cos(thetaHat);
  W[1][0] = .5*sin(thetaHat);
  W[1][1] = .5*sin(thetaHat);

  //Aggiorno Q (solo gli elementi variabili)
  Q[0][0] = KR*abs(uRe);
  Q[1][1] = KL*abs(uLe);
  
  // Calcolo matrice covarianza a priori
  Pmeno = mSum(mProd(mProd(F,P),trasposta(F)),mProd(mProd(W,Q),trasposta(W))); // Pmeno = F*P*F' + W*Q*W'

 // STIMA FILTRO DI KALMAN ESTESO: PASSO di CORREZIONE
  if (millis()-tempoUltimaMisura >= tStep) // attuo la correzione solo se ho le misure (che arrivano ogni tStep ms)
  {
    tempoUltimaMisura = millis(); // memorizzo il tempo in cui ho fatto l'ultima misura
    nStime++; // incremento il contatore delle stime fatte
    
    // per ogni landmark calcolo misura vera, attesa, la riga della 
    // matrice giacobiana H e l'innovazione corrispondente
    for (int indLandmark=0; indLandmark<nL; indLandmark++) 
    {
      //misureLandmark[indLandmark] = sqrt(pow(x-Landmark[indLandmark][0],2) + pow(y-Landmark[indLandmark][1],2))+Gaussian(0,sigmaLandmark);
      //misureAtteseLandmark[indLandmark] = sqrt(pow(xHatMeno-Landmark[indLandmark][0],2) + pow(yHatMeno-Landmark[indLandmark][1],2));
      AngoloLandmark[indLandmark]= atan2(Landmark[indLandmark][1]-y,Landmark[indLandmark][0]-x)- theta + Gaussian(0,sigmaTheta0);
      AngoloLandmarkAtteso[indLandmark]=atan2(Landmark[indLandmark][1]-yHatMeno,Landmark[indLandmark][0]-xHatMeno) - thetaHat+nGiriT*2*PI;
          if (abs(AngoloLandmarkAtteso[indLandmark]+2*PI-AngoloLandmark[indLandmark]) < abs(AngoloLandmarkAtteso[indLandmark]-AngoloLandmark[indLandmark]))
    {
      AngoloLandmarkAtteso[indLandmark] = AngoloLandmarkAtteso[indLandmark]+2*PI;
      nGiriT += 1;
    }
    else
    {
      if (abs(AngoloLandmarkAtteso[indLandmark]-2*PI-AngoloLandmark[indLandmark]) < abs(AngoloLandmarkAtteso[indLandmark]-AngoloLandmark[indLandmark]))
      {
        AngoloLandmarkAtteso[indLandmark] = AngoloLandmarkAtteso[indLandmark]-2*PI;
        nGiriT += -1;
      }
    }
    //  DeltaX = xHatMeno-Landmark[indLandmark][0];
    //  DeltaY = yHatMeno-Landmark[indLandmark][1];
    //  DeltaXY = sqrt(pow(DeltaX,2)+pow(DeltaY,2));
      DeltaY=Landmark[indLandmark][0]-xHatMeno;
      DeltaX=Landmark[indLandmark][1]-yHatMeno;
      DeltaXY=pow(DeltaX,2)+pow(DeltaY,2);
      H[indLandmark][0] = DeltaX/DeltaXY;
      H[indLandmark][1] = (-DeltaY)/DeltaXY;
      H[indLandmark][2] = -1;
      //innovazione[indLandmark][0] = misureLandmark[indLandmark]-misureAtteseLandmark[indLandmark];
      // innovazione = zk+1 - z=h(xhatmeno,0)
        innovazione[indLandmark][0]=AngoloLandmark[indLandmark]-AngoloLandmarkAtteso[indLandmark];
    }
    
    
    // Calcolo guadagno Kalman e aggiorno covarianza
    K = mProd(mProd(Pmeno,trasposta(H)),invMat(mSum(mProd(mProd(H,Pmeno),trasposta(H)),Rs)));
    P = mProd(mSum(idMat(3,1),mProd(idMat(3,-1),mProd(K,H))),Pmeno);

    // Correggo la stima    
    correzione = mProd(K,innovazione);
    xHat = xHatMeno + correzione[0][0];
    yHat = yHatMeno + correzione[1][0];
    thetaHat = thetaHatMeno + correzione[2][0];    
  }
  else  // se non ho misure non correggo nulla
  {
    xHat = xHatMeno;
    yHat = yHatMeno;
    thetaHat = thetaHatMeno;
    P = Pmeno;
  }
// FINE EKF
// Disegno i landmark con dei triangoli bianchi col contorno rosso e l'identificativo del landmark all'interno
  stroke(255,0,0);
  strokeWeight(2);  
  stroke(0);
  strokeWeight(1);
  
  popMatrix();

  textSize(20);
  fill(0,0,255);
  text("v1 (pixel/s) = ",10,20); 
  text(v1,200,20);
  text("v2 (gradi/s) = ",10,50); 
  text(v2*180/PI,200,50);
  
  fill(255,0,0);  
  text("x = ",10,160); 
  text(x,80,160);
  text("y = ",10,190); 
  text(y,80,190);
  text("theta = ",10,220); 
  text(theta*180/PI,100,220);  

  fill(255,255,255);
  text("tempo = ",10,110); 
  text(tempo,120,110);  

  fill(0,0,255);
  text("omegaR (gradi/s) = ",650,20); 
  text(omegaL*180/PI,900,20);
  text("omegaL (gradi/s) = ",650,50); 
  text(omegaL*180/PI,900,50);
  
  fill(255,0,0);
  text("xDes = ",700,100); 
  text(xDes,800,100);
  text("yDes = ",700,130); 
  text(yDes,800,130);

  fill(255,255,0);  
  text("xHat = ",10,280); 
  text(xHat,120,280);
  text("yHat = ",10,310); 
  text(yHat,120,310);
  text("thetaHat = ",10,340); 
  text(thetaHat*180/PI,160,340);  

  fill(255,255,255);
  text("nStime = ",10,390); 
  text(nStime,120,390);  

  fill(255,255,255);
  text("tStep (ms) = ",10,420); 
  text(tStep,150,420);  

  fill(255,255,0);  
  text("P = ",10,460); 
  text(P[0][0],10,490); text(P[0][1],100,490); text(P[0][2],190,490);
  text(P[1][0],10,520); text(P[1][1],100,520); text(P[1][2],190,520); 
  text(P[2][0],10,550); text(P[2][1],100,550); text(P[2][2],190,550);  
  
}

void robot(float x, float y, float theta, int colore)
{
  stroke(0);
  strokeWeight(1);
// funzione che disegna uniciclo in (x,y) con orientamento theta      
  pushMatrix();
  translate(x,-y);
  rotate(-theta);
  if (colore==1)
  {
    fill(255,0,0);
  }
  else
  {
    fill(255,255,0);
  }
  ellipse(0,0,2*R,2*R); // il robot
  fill(0,0,255);  
  rect(-r,-d/2-w/2,2*r,w);
  rect(-r,d/2-w/2,2*r,w);
  fill(255);
  ellipse(.8*R,0,.2*R,.2*R);
  ellipse(-.8*R,0,.2*R,.2*R);  
  fill(0,255,0);
  triangle(-.1*R,.3*R,-.1*R,-.3*R,.5*R,0);
  popMatrix();
}
void robot1(float x, float y, float theta, int colore)
{
  stroke(0);
  strokeWeight(1);
// funzione che disegna uniciclo in (x,y) con orientamento theta      
  pushMatrix();
  translate(x,-y);
  rotate(-theta);
     fill(0,140,0);
  arc(0, 0, rMax, rMax, -betaMax, betaMax, PIE);
  if (colore==1)
  {
    fill(255,0,0);
  }
  else
  {
    fill(255,255,0);
  }
  ellipse(0,0,2*R,2*R); // il robot
  fill(0,0,255);  
  rect(-r,-d/2-w/2,2*r,w);
  rect(-r,d/2-w/2,2*r,w);
  fill(255);
  ellipse(.8*R,0,.2*R,.2*R);
  ellipse(-.8*R,0,.2*R,.2*R);  
  fill(0,255,0);
  triangle(-.1*R,.3*R,-.1*R,-.3*R,.5*R,0);
  popMatrix();
}

/******************************************************
/******************************************************
  DA QUI IN POI CI SONO FUNZIONI DI SERVIZIO: 
  1) CALCOLO ERRORE GAUSSIANO
  2) ALGEBRA MATRICIALE
/******************************************************
/******************************************************/

float Gaussian(float media, float stDev) // Restituisce variabile N(media,stDev^2) approssimata sfruttando il teorema del limite centrale
{
  float somma = 0;
  for (int k=0; k<27; k+=1) // 27 in modo che sqrt(3/27)=1/3
  {
    somma = somma + random(-stDev/3,stDev/3);
  }
  return media + somma;
}

float[][] mProd(float[][] A,float[][] B) // Calcola prodotto di due matrici A e B (si assume, senza controllarlo, che numero colonne A = numero righe B!)
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

float[][] mSum(float[][] A,float[][] B) // Calcola la somma di due matrici A e B (si assume, senza controllarlo, che A e B abbiano stesse dimensioni!)
{
  int nA = A.length;
  int nB = A[0].length;
  
  float[][] C = new float[nA][nB]; 

  for (int i=0; i < nA; i++) 
  {
    for (int j=0; j < nB; j++) 
    {  
      C[i][j] = A[i][j] + B[i][j];
    }
  }
  return C;
}

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


float[][] minore(float[][] A, int i, int j) // Determina il minore (i,j) di una matrice A (si assume, senza controllarlo, che A sia quadrata!)
{
  int nA = A.length;
  float[][] C = new float[nA-1][nA-1];
  
  for (int iM = 0; iM < i; iM++)
  {
    for (int jM = 0; jM < j; jM++)
    {
      C[iM][jM] = A[iM][jM];
    } 
    for (int jM = j; jM < nA-1; jM++)
    {
      C[iM][jM] = A[iM][jM+1];
    } 
  }
  for (int iM = i; iM < nA-1; iM++)
  {
    for (int jM = 0; jM < j; jM++)
    {
      C[iM][jM] = A[iM+1][jM];
    } 
    for (int jM = j; jM < nA-1; jM++)
    {
      C[iM][jM] = A[iM+1][jM+1];
    } 
  }
  return C;
}


float det(float[][] A) // Calcola il determinante di A (si assume, senza controllarlo, che A sia quadrata!)
{
  int nA = A.length;
  float determinante = 0;
  
  if (nA == 1)
  {
    determinante = A[0][0];
  }
  else
  {
    for (int j=0; j < nA; j++) 
    {
      determinante = determinante + A[0][j]*pow(-1,j)*det(minore(A,0,j));
    }
  }
  return determinante;
}


float[][] invMat(float[][] A) // Calcola l'inversa di una matrice A (si assume, senza controllarlo, che A sia quadrata!)
{
  int nA = A.length;
  float[][] C = new float[nA][nA];
  float detA = det(A);

/*
  if (abs(detA)<0.001) // Per evitare casi singolari con determinanti vicini a 0
  {
    if (detA>0)
    {
      detA = 0.001;
    }
    else
    {
      detA = -0.001;
    }
  }
*/  
  
  if (nA == 1)
  {
    C[0][0] = 1/detA;
  }
  else
  {
    for (int i=0; i < nA; i++) 
    {
      for (int j=0; j < nA; j++) 
      {
        C[j][i] = pow(-1,i+j)*det(minore(A,i,j))/detA;
      }
    }
  }
  return C;
}

float[][] idMat(int nA, float sigma) // Assegna una matrice identità di ordine nA moltiplicata per una costante sigma
{
  float[][] I = new float[nA][nA]; 

  for (int i=0; i < nA; i++) 
  {
    for (int j=0; j < nA; j++) 
    {  
      I[i][j] = 0;
    }
    I[i][i] = sigma;
  }
  return I;
}





//switch(ans){
//  //
//  case 0: 
//  stroke(255,0,0);
//  strokeWeight(2);  
//    fill(10,10,10);
//    triangle(Landmark[indLandmark][0]-15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0]+15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0],-Landmark[indLandmark][1]-15);
//    textSize(10);
//    fill(0,0,0);
//    text("L",Landmark[indLandmark][0]-5,-Landmark[indLandmark][1]+8);
//    text(indLandmark+1,Landmark[indLandmark][0]+1,-Landmark[indLandmark][1]+8);
//    noStroke();
//    break;
//    //
//    case 1:  
//    fill(10,10,10);
//    triangle(Landmark[indLandmark][0]-15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0]+15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0],-Landmark[indLandmark][1]-15);
//    textSize(10);
//    fill(0,0,0);
//    text("L",Landmark[indLandmark][0]-5,-Landmark[indLandmark][1]+8);
//    text(indLandmark+1,Landmark[indLandmark][0]+1,-Landmark[indLandmark][1]+8);
//    noStroke();
//    break;
//    //
//    case 2:  
//    fill(10,10,10);
//    triangle(Landmark[indLandmark][0]-15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0]+15,-Landmark[indLandmark][1]+15,Landmark[indLandmark][0],-Landmark[indLandmark][1]-15);
//    textSize(10);
//    fill(0,0,0);
//    text("L",Landmark[indLandmark][0]-5,-Landmark[indLandmark][1]+8);
//    text(indLandmark+1,Landmark[indLandmark][0]+1,-Landmark[indLandmark][1]+8);
//    noStroke();
//    break;
//}

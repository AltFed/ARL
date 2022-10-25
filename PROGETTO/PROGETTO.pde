float lw=30,l1=50,l2=200,l3=100,l4=100,l5=50,l6=30;  //link
float g = 50;
float xd=0,yd=0,zd=0;
float q[] = {0,0,0,0,0,0};
float xBase, yBase;
float eyeY,segno = 1;
float alfa=0,beta=0,theta=0;//pinza orientata verso il basso
float R3_6[][];
void setup(){
  size(1000,800,P3D);
  strokeWeight(2);
  stroke(0);
  xBase=width/2;
  yBase=height/2;
  initR36();
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
   // funzione per il movimento 
    muovi();
   //  funzione per definire il manipolatore
    robot(q); 
  // funzione per la grafica
   popMatrix();
   graphic(xd,yd,zd,alfa,beta,theta);
}
void initR36(){
  R36[0][0]=
}

void robot(float q[]){
  translate(xBase, yBase, 0);
//--------link 0--------
  box(l1,l1,l1);           
  fill(255,0,0);
//link 1
  translate(0,-l1,0);
  box(l1,l1,l1);
  
//struttura per il link 2
  translate(l1/2-lw/2,-l1/2-lw/2,0);
  box(lw,lw,g);     
  
//link 2  
  translate(l2/2+lw/2,0,0);
  box(l2,lw,lw);          
  
//struttura link 3
  translate(l2/2-lw/2,0,0);
  box(lw,lw,g);          

//link3  
  translate(0,-l3/2-lw/2,0);
  box(lw,l3,lw); 
  
//link 4
  translate(0,-l4,0);
  box(lw,l4,lw);
  
//struttura link 5
  translate(0,-l4/2-lw/2,0);
  box(lw,lw,g);
  
//link 5
  translate(l5/2+lw/2,0,0);
  box(l5,lw,lw);
  
//link 6
  translate(l5/2+lw/2,0,0);
  rotateY(beta);
  box(l6,lw,lw);
}
void muovi(){
  //cinematica inversa 
}
void graphic(float xd,float yd,float zd,float alfa,float beta,float theta){
  textSize(20);
  fill(255);
  // grafica punto desiderato 
  //tali coordinate vanno scritte rispetto alla base aggiornare
  text("xd = ",10,20);
  text(xd,50,20);
  text("yd = ",10,40);
  text(yd,50,40);
  text("zd = ",10,60);
  text(zd,50,60);
  // orientamento della pinza
  text("alfa = ",110,20);
  text(alfa,153,20);
  text("beta = ",110,40);
  text(beta,160,40);
  text("theta = ",110,60);
  text(theta,170,60);
  if(segno==1){
    text("gomito alto ",10,80);
  }else if ( segno == -1){
    text("gomito basso",10,80);
  }
}

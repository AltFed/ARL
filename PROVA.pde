/* Questo sketch permette di disegnare un robot con un solo
giunto, di spostarlo col mouse e di attuare il giunto con 
le frecce destra e sinistra */

float L1w = 30; // lato quadrato sezione link 1
float L1 = 200; // lunghezza link 1
float L2w = 30; // lato quadrato sezione link 2
float L2 = 200; // lunghezza link 2
float L3w = 30; // lato quadrato sezione link 3
float L3 = 200; // lunghezza link 3
float L4w=30;
float L4=15;


float xBase;
int   index;
float yBase;
float q[]={0,0,0}; // angoli giunti

void setup() 
{
  size(1000, 800, P3D);
  strokeWeight(2);
  stroke(0);
  xBase = width/2;
  yBase = height/2;  
}

void draw() 
{
  background(50);
  lights();
 
  if (mousePressed)
  {
    xBase = mouseX;
    yBase = mouseY;
  }
  
  if (keyPressed)
  {
    if(key == '0'){
      index=0;
    }
    if(key == '1'){
      index=1;
    }
    if(key == '2'){
      index=2;
    }
      
    if (keyCode == LEFT)
    {
      if(index==1){
       if(-q[index]-L3/2+L2w/2<0){
        q[index] -=1;
      }
      }else{
      q[index] -= .01;
    }
    }
    if (keyCode == RIGHT)
    {
      if(index==1){
        if(q[index]-L3/2+L2w/2<0){
        q[index] +=1;
      }
      }else{
      q[index] += .01;
    }
  }
  }
  
  textSize(25);
  fill(255,0,0);
  text("giunto = ",500,30);
  text(index,600,30);
  text("theta 0 = ",10,20); 
  text(q[0]*180/PI,100,20);
  text(" traslazione 1 = ",10,50); 
  text(q[1],170,50);
  text(" traslazione 2 = ",10,80); 
  text(q[2],170,80);
  
  
  
  robot(q);
  
} 
 
void robot(float q[])
{
  fill(255);
  
  // Primo link (parallelepipedo)
  translate(xBase,yBase-L1/2);
  box(L1w,L1,L1w);
  
  // Secondo link (parallelepipedo)
  rotateY(q[0]);
  translate(L2/2-L1w/2,-L1/2-L2w/2,0);
  box(L2,L2w,L2w); 
  // Terzo link
  translate(L2-(L2/2-L1w/2)-L3w,q[1],0);
  box(L3w,L3,L3w);
  // Quarto link
  translate(-L4/2-q[2],L3/2+L4w/2,0);
  box(L4,L4w,L4w);
  //
  translate(L4+2*q[2],0,0);
  box(L4,L4w,L4w);
}

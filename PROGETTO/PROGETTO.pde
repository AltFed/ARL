float lw=30,l1=50,l2=200,l3=100,l4=100,l5=50,l6=30;  //link
float g = 50;
float xd,yd,zd;
float q[] = {0,0,0,0,0,0};
float xBase, yBase;
float eyeY;
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
    if(key=='x'){
      xd -= 5;
    }
    if(key=='X'){
      xd += 5;
    }
    if(key=='y'){
      yd -= 5;
    }
    if(key=='Y'){
      yd += 5;
    }
    if(key=='z'){
      zd -= 5;
    }
    if(key=='Z'){
      zd += 5;
    }
  }
  robot(q);
 
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
  box(l6,lw,lw);
  
  
  
}

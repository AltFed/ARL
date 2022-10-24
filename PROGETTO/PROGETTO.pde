float lw=30,l1=50,l2=100,l3=100,l4=50,l5=30,l6=30;  //link
float xd,yd,zd;
void setup()
{
  
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
  if(keyPressed){
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
}

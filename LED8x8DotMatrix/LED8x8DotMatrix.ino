#include "LedControl.h"

/*
 Now we need a LedControl to work with.
 pin 12 is connected to the CLK 
 pin 11 is connected to LOAD or CS
 pin 10 is connected to the DataIn 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(10,12,11,1);

/* we always wait a bit between updates of the display */
unsigned long delaytime=500;
unsigned long blinktime=200;



void setup() {

  Serial.begin(115200);

  lc.shutdown(0,false);
  // Set brightness to a medium value
  lc.setIntensity(0,6);
  // Clear the display
  lc.clearDisplay(0);  

  drawSpaceInvader();

}

void loop() {

  
  //drawFaces();
}

/*
00011000
00111100
01111110
11011011
11111111
00100100
01011010
10000001

8 Bit SNES Sprite Art - https://georgjz.github.io/snesaa03/

*/
void drawSpaceInvader(){

  byte si[8]= {0x18,0x3c,0x7e,0xdb,0xff,0x24,0x5a,0x81};  
  int len = sizeof(si) / sizeof(si[0]); // len type could also be size_t
  byte b[len]; 

  shiftUp(si,b,len,i);

  /*
  for(int i=1; i<9; i++)
  {
    //shiftRight(si,b,len,1);
    //shiftDown(si,b,len,i);
    //shiftUp(si,b,len,i);
  
    Serial.println("");
  
    for(int j=0; j<8; j++)
    {    
      Serial.println(b[j],BIN);
      lc.setColumn(0,j,b[j]);
    }
    delay(500);
  }
  */

  /*

  // V-Wipe/Blind
  for(int s=0; s<9; s++)
  {    
    for(int i=0; i<8; i++)
    {
      byte v = (i >= s) ? si[i] : 0x00; 
      lc.setColumn(0,i,v);
    }
    delay(delaytime);
  }

  // flash n times
  for(int i=0; i<8; i++)
  {    
    lc.setColumn(0,i,si[i]);
  }
  delay(delaytime);

  for(int i=0; i<8; i++)
  {
     int b = (i%2) ? 6 : 0;
     lc.setIntensity(0,b);
     delay(blinktime);
  }

  // Clear the display
  lc.clearDisplay(0);  

  delay(delaytime);
  */
}



/**
 * Scroll - Vertical Up
 */
void shiftUp(byte *in, byte *out, int sz, int n)
{

  for(int i = 0; i<sz; i++)
  {
    out[i+n] = in[i];
  }
  return out;
}

/**
 * Scroll - Vertical Down
 */
void shiftDown(byte *in, byte *out, int sz, int n)
{

  for(int i = 0; i<=n; i++)
  {
    out[i] = 0x00;
  }
  for(int i = n; i<sz; i++)
  {
    out[i] = in[i-n];
  }
  return out;
}


/**
 * Scroll - Right Shift
 */
void shiftRight(byte *in, byte *out, int sz, int n)
{
  for(int i=0; i<sz; i++)
  {    
    //Serial.println(in[i],BIN);
    //byte b = (in[i]);
    byte b = (in[i] << n);
    Serial.println(b,BIN);
    out[i] = b;
  }
  return out;
}

/**
 * Scroll - Left Shift
 */
void shiftLeft(byte *in, byte *out, int sz, int n)
{
  for(int i=8; i<sz; i--)
  {
    byte b = (in[i] << n);
    Serial.println(b,BIN);
    out[i] = b;
  }
  return out;
}


void drawFaces(){

  // happy face
  byte hf[8]= {B00111100,B01000010,B10100101,B10000001,B10100101,B10011001,B01000010,B00111100};
  // neutral face
  byte nf[8]={B00111100, B01000010,B10100101,B10000001,B10111101,B10000001,B01000010,B00111100};
  // sad face
  byte sf[8]= {B00111100,B01000010,B10100101,B10000001,B10011001,B10100101,B01000010,B00111100};

  // Display sad face
  lc.setColumn(0,0,sf[0]);
  lc.setColumn(0,1,sf[1]);
  lc.setColumn(0,2,sf[2]);
  lc.setColumn(0,3,sf[3]);
  lc.setColumn(0,4,sf[4]);
  lc.setColumn(0,5,sf[5]);
  lc.setColumn(0,6,sf[6]);
  lc.setColumn(0,7,sf[7]);
  delay(delaytime);
  
  // Display neutral face
  lc.setColumn(0,0,nf[0]);
  lc.setColumn(0,1,nf[1]);
  lc.setColumn(0,2,nf[2]);
  lc.setColumn(0,3,nf[3]);
  lc.setColumn(0,4,nf[4]);
  lc.setColumn(0,5,nf[5]);
  lc.setColumn(0,6,nf[6]);
  lc.setColumn(0,7,nf[7]);
  delay(delaytime);
  
  // Display happy face
  lc.setColumn(0,0,hf[0]);
  lc.setColumn(0,1,hf[1]);
  lc.setColumn(0,2,hf[2]);
  lc.setColumn(0,3,hf[3]);
  lc.setColumn(0,4,hf[4]);
  lc.setColumn(0,5,hf[5]);
  lc.setColumn(0,6,hf[6]);
  lc.setColumn(0,7,hf[7]);
  delay(delaytime);
}


void writeArduinoOnMatrix() {
  /* here is the data for the characters */
  byte a[8]={B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000};
  byte b[8]={B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000};
  byte c[8]={B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000};
  byte d[8]={B00000000,B00000000,B00011000,B00011000,B00011000,B00011000,B00000000,B00000000};
  byte e[8]={B00000000,B00011000,B00011000,B00011000,B00011000,B00011000,B00011000,B00000000};
  byte f[8]={B00011000,B00011000,B00011000,B00011000,B00011000,B00011000,B00011000,B00011000};
  byte g[8]={B00111100,B00100100,B00100100,B00100100,B00100100,B00100100,B00100100,B00111100};
  byte h[8]={B01111110,B01000010,B01000010,B01000010,B01000010,B01000010,B01000010,B01111110};
  byte i[8]={B11111111,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B11111111};
  byte j[8]={B00000000,B11111111,B10000001,B10000001,B10000001,B10000001,B11111111,B00000000};
  byte k[8]={B00000000,B00000000,B11111111,B10000001,B10000001,B11111111,B00000000,B00000000};
  byte l[8]={B00000000,B00000000,B00000000,B11111111,B11111111,B00000000,B00000000,B00000000};
  byte m[8]={B00000000,B00000000,B00000000,B01111110,B01111110,B00000000,B00000000,B00000000};
  byte n[8]={B00000000,B00000000,B00000000,B00111100,B00111100,B00000000,B00000000,B00000000};
  byte o[8]={B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000};
  byte p[8]={B00000000,B00000000,B00111100,B00100100,B00100100,B00111100,B00000000,B00000000};
  byte q[8]={B00000000,B01111110,B01000010,B01000010,B01000010,B01000010,B01111110,B00000000};
  byte r[8]={B11111111,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B11111111};
  byte s[8]={B00000000,B01111110,B01000010,B01000010,B01000010,B01000010,B01111110,B00000000};
  byte t[8]={B00000000,B00000000,B00111100,B00100100,B00100100,B00111100,B00000000,B00000000};
  byte u[8]={B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000};

  /* now display them one by one with a small delay */
  lc.setColumn(0,0,a[0]);
  lc.setColumn(0,1,a[1]);
  lc.setColumn(0,2,a[2]);
  lc.setColumn(0,3,a[3]);
  lc.setColumn(0,4,a[4]);
  lc.setColumn(0,5,a[5]);
  lc.setColumn(0,6,a[6]);
  lc.setColumn(0,7,a[7]);
  delay(delaytime);
  lc.setColumn(0,0,b[0]);
  lc.setColumn(0,1,b[1]);
  lc.setColumn(0,2,b[2]);
  lc.setColumn(0,3,b[3]);
  lc.setColumn(0,4,b[4]);
  lc.setColumn(0,5,b[5]);
  lc.setColumn(0,6,b[6]);
  lc.setColumn(0,7,b[7]);
  delay(delaytime);
  lc.setColumn(0,0,c[0]);
  lc.setColumn(0,1,c[1]);
  lc.setColumn(0,2,c[2]);
  lc.setColumn(0,3,c[3]);
  lc.setColumn(0,4,c[4]);
  lc.setColumn(0,5,c[5]);
  lc.setColumn(0,6,c[6]);
  lc.setColumn(0,7,c[7]);
  delay(delaytime);
  lc.setColumn(0,0,d[0]);
  lc.setColumn(0,1,d[1]);
  lc.setColumn(0,2,d[2]);
  lc.setColumn(0,3,d[3]);
  lc.setColumn(0,4,d[4]);
  lc.setColumn(0,5,d[5]);
  lc.setColumn(0,6,d[6]);
  lc.setColumn(0,7,d[7]);
  delay(delaytime);
  lc.setColumn(0,0,e[0]);
  lc.setColumn(0,1,e[1]);
  lc.setColumn(0,2,e[2]);
  lc.setColumn(0,3,e[3]);
  lc.setColumn(0,4,e[4]);
  lc.setColumn(0,5,e[5]);
  lc.setColumn(0,6,e[6]);
  lc.setColumn(0,7,e[7]);
  delay(delaytime);
  lc.setColumn(0,0,f[0]);
  lc.setColumn(0,1,f[1]);
  lc.setColumn(0,2,f[2]);
  lc.setColumn(0,3,f[3]);
  lc.setColumn(0,4,f[4]);
  lc.setColumn(0,5,f[5]);
  lc.setColumn(0,6,f[6]);
  lc.setColumn(0,7,f[7]);
  delay(delaytime);
  lc.setColumn(0,0,g[0]);
  lc.setColumn(0,1,g[1]);
  lc.setColumn(0,2,g[2]);
  lc.setColumn(0,3,g[3]);
  lc.setColumn(0,4,g[4]);
  lc.setColumn(0,5,g[5]);
  lc.setColumn(0,6,g[6]);
  lc.setColumn(0,7,g[7]);
  delay(delaytime);
  lc.setColumn(0,0,h[0]);
  lc.setColumn(0,1,h[1]);
  lc.setColumn(0,2,h[2]);
  lc.setColumn(0,3,h[3]);
  lc.setColumn(0,4,h[4]);
  lc.setColumn(0,5,h[5]);
  lc.setColumn(0,6,h[6]);
  lc.setColumn(0,7,h[7]);
  delay(delaytime);
  lc.setColumn(0,0,i[0]);
  lc.setColumn(0,1,i[1]);
  lc.setColumn(0,2,i[2]);
  lc.setColumn(0,3,i[3]);
  lc.setColumn(0,4,i[4]);
  lc.setColumn(0,5,i[5]);
  lc.setColumn(0,6,i[6]);
  lc.setColumn(0,7,i[7]);
  delay(delaytime);
  lc.setColumn(0,0,j[0]);
  lc.setColumn(0,1,j[1]);
  lc.setColumn(0,2,j[2]);
  lc.setColumn(0,3,j[3]);
  lc.setColumn(0,4,j[4]);
  lc.setColumn(0,5,j[5]);
  lc.setColumn(0,6,j[6]);
  lc.setColumn(0,7,j[7]);
  delay(delaytime);
  lc.setColumn(0,0,k[0]);
  lc.setColumn(0,1,k[1]);
  lc.setColumn(0,2,k[2]);
  lc.setColumn(0,3,k[3]);
  lc.setColumn(0,4,k[4]);
  lc.setColumn(0,5,k[5]);
  lc.setColumn(0,6,k[6]);
  lc.setColumn(0,7,k[7]);
  delay(delaytime);
  lc.setColumn(0,0,l[0]);
  lc.setColumn(0,1,l[1]);
  lc.setColumn(0,2,l[2]);
  lc.setColumn(0,3,l[3]);
  lc.setColumn(0,4,l[4]);
  lc.setColumn(0,5,l[5]);
  lc.setColumn(0,6,l[6]);
  lc.setColumn(0,7,l[7]);
  delay(delaytime);
  lc.setColumn(0,0,m[0]);
  lc.setColumn(0,1,m[1]);
  lc.setColumn(0,2,m[2]);
  lc.setColumn(0,3,m[3]);
  lc.setColumn(0,4,m[4]);
  lc.setColumn(0,5,m[5]);
  lc.setColumn(0,6,m[6]);
  lc.setColumn(0,7,m[7]);
  delay(delaytime);
  lc.setColumn(0,0,n[0]);
  lc.setColumn(0,1,n[1]);
  lc.setColumn(0,2,n[2]);
  lc.setColumn(0,3,n[3]);
  lc.setColumn(0,4,n[4]);
  lc.setColumn(0,5,n[5]);
  lc.setColumn(0,6,n[6]);
  lc.setColumn(0,7,n[7]);
  delay(delaytime);
  lc.setColumn(0,0,o[0]);
  lc.setColumn(0,1,o[1]);
  lc.setColumn(0,2,o[2]);
  lc.setColumn(0,3,o[3]);
  lc.setColumn(0,4,o[4]);
  lc.setColumn(0,5,o[5]);
  lc.setColumn(0,6,o[6]);
  lc.setColumn(0,7,o[7]);
  delay(delaytime);
  lc.setColumn(0,0,p[0]);
  lc.setColumn(0,1,p[1]);
  lc.setColumn(0,2,p[2]);
  lc.setColumn(0,3,p[3]);
  lc.setColumn(0,4,p[4]);
  lc.setColumn(0,5,p[5]);
  lc.setColumn(0,6,p[6]);
  lc.setColumn(0,7,p[7]);
  delay(delaytime);
  lc.setColumn(0,0,q[0]);
  lc.setColumn(0,1,q[1]);
  lc.setColumn(0,2,q[2]);
  lc.setColumn(0,3,q[3]);
  lc.setColumn(0,4,q[4]);
  lc.setColumn(0,5,q[5]);
  lc.setColumn(0,6,q[6]);
  lc.setColumn(0,7,q[7]);
  delay(delaytime);
  lc.setColumn(0,0,r[0]);
  lc.setColumn(0,1,r[1]);
  lc.setColumn(0,2,r[2]);
  lc.setColumn(0,3,r[3]);
  lc.setColumn(0,4,r[4]);
  lc.setColumn(0,5,r[5]);
  lc.setColumn(0,6,r[6]);
  lc.setColumn(0,7,r[7]);
  delay(delaytime);
  lc.setColumn(0,0,s[0]);
  lc.setColumn(0,1,s[1]);
  lc.setColumn(0,2,s[2]);
  lc.setColumn(0,3,s[3]);
  lc.setColumn(0,4,s[4]);
  lc.setColumn(0,5,s[5]);
  lc.setColumn(0,6,s[6]);
  lc.setColumn(0,7,s[7]);
  delay(delaytime);
  lc.setColumn(0,0,t[0]);
  lc.setColumn(0,1,t[1]);
  lc.setColumn(0,2,t[2]);
  lc.setColumn(0,3,t[3]);
  lc.setColumn(0,4,t[4]);
  lc.setColumn(0,5,t[5]);
  lc.setColumn(0,6,t[6]);
  lc.setColumn(0,7,t[7]);
  delay(delaytime);
  lc.setColumn(0,0,u[0]);
  lc.setColumn(0,1,u[1]);
  lc.setColumn(0,2,u[2]);
  lc.setColumn(0,3,u[3]);
  lc.setColumn(0,4,u[4]);
  lc.setColumn(0,5,u[5]);
  lc.setColumn(0,6,u[6]);
  lc.setColumn(0,7,u[7]);
  delay(delaytime);
}

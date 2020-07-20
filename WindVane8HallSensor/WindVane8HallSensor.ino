/**
 * Weather Vane - 8 Magnetic Hall Sensor Switch
 * 
 * A ring of 8 magnetic digital hall sensors 
 * attached to digital pins 3-10 are activated 
 * by a rotating magnet attached to vane shaft
 *  
 */


// current and previous active sensor pin
int active = NULL;
int lastActive = NULL;

// pin order direction labels
char d1[] = "NE";
char d2[] = "SE";
char d3[] = "E";
char d4[] = "S";
char d5[] = "N";
char d6[] = "W";
char d7[] = "NW";
char d8[] = "SW";

char * directionLabel[] = { d1, d2, d3, d4, d5, d6, d7, d8 };


void setup() {

  Serial.begin(115200);

  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);
  pinMode(10,INPUT_PULLUP);

}

void loop() {

  int v;
  active = 0;

  for(int i = 3; i <= 10; i++)
  {
    v = digitalRead(i);

    //Serial.print(v);
    //Serial.print("    \t");

    if (v == 0)
    {
      active = i;
      Serial.print(i);
      Serial.print("\t");
      Serial.println(directionLabel[i-3]);
    }
  }
  if (active == 0) // magnet between sensor positions
  {
      Serial.print(lastActive);
      Serial.print("\t");
      Serial.println(directionLabel[lastActive-3]);
    
  }
  lastActive = active;

  delay(200);
}

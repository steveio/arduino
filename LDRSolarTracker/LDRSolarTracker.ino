/**
 * Dual Axis Solar Tracker
 * 
 * 4x LDR (GL5537) arranged in + configuration 
 * Interfaced to CD4051be Analogue Multiplexer
 * 
 * References:
 * http://tok.hakynda.com/article/detail/144/cd4051be-cmos-analog-multiplexersdemultiplexers-with-logic-level-conversion-analog-input
 * https://theiotprojects.com/dual-axis-solar-tracker-arduino-project-using-ldr-servo-motors/
 * 
 */

const int selectPins[3] = {2, 3, 4}; // S0~2, S1~3, S2~4
const int zInput = A0; // Connect common (Z) to A0 (analog input)

void setup() 
{
  Serial.begin(115200); // Initialize the serial port

  // Using Port Manipulation ( https://www.arduino.cc/en/Reference/PortManipulation ) 
  DDRD = DDRD | B00011100; // set select pins as OUTPUT
  PORTD = B00011100; // set pins 2,3,4 HIGH

}

void loop() 
{

  // LDR sensor array + config -> multiplexer IO (channels)
  // 4 7
  // 6 5

  // To read CD4051B channels 0-7:
  //
  // Channel 4 = 0100
  // digitalWrite(2, 0);
  // digitalWrite(3, 0);
  // digitalWrite(4, 1);
  //
  // Or using direct port manipulation
  //  PORTD = B00010000;


  PORTD = B00010000;
  int lt = analogRead(zInput); // top left

  PORTD = B00011100;
  int rt = analogRead(zInput); // top right

  PORTD = B00011000;
  int ld = analogRead(zInput); // down left

  PORTD = B00010100;
  int rd = analogRead(zInput); // down right

  Serial.print(lt);
  Serial.print("\t");
  Serial.print(rt);

  Serial.println("");

  Serial.print(ld);
  Serial.print("\t");
  Serial.print(rd);

  Serial.println("\n");

  int dtime = 10; int tol = 90; // dtime=diffirence time, tol=toleransi
  int avt = (lt + rt) / 2; // average value top
  int avd = (ld + rd) / 2; // average value down
  int avl = (lt + ld) / 2; // average value left
  int avr = (rt + rd) / 2; // average value right
  int dvert = avt - avd; // check the diffirence of up and down
  int dhoriz = avl - avr;// check the diffirence og left and rigt


  Serial.println("avt\tavd\tavl\tavr\tdvert\tvhoriz\t");
  Serial.println("---\t---\t---\t---\t---\t---\t");

  Serial.print(avt);
  Serial.print("\t");
  Serial.print(avd);
  Serial.print("\t");
  Serial.print(avl);
  Serial.print("\t");
  Serial.print(avr);
  Serial.print("\t");
  Serial.print(dvert);
  Serial.print("\t");
  Serial.print(dhoriz);

  Serial.println("\n");

  delay(1000);

  /* Code to driver dual axis servo motors  
   *  
  #include <Servo.h> 
  
  
  Servo horizontal; // horizontal servo
  int servoh = 180; 
  int servohLimitHigh = 175;
  int servohLimitLow = 5;
  // 65 degrees MAX
  
  Servo vertical; // vertical servo
  int servov = 45; 
  int servovLimitHigh = 60;
  int servovLimitLow = 1;
  

  if (-1*tol > dvert || dvert > tol) 
  {
    if (avt > avd)
    {
      servov = ++servov;
      if (servov > servovLimitHigh)
      {
        servov = servovLimitHigh;
      }
    } else if (avt < avd)
    {
      servov= --servov;
      if (servov < servovLimitLow)
      { 
        servov = servovLimitLow;
      }
    }
    vertical.write(servov);
  }
  
  if (-1*tol > dhoriz || dhoriz > tol) // check if the diffirence is in the tolerance else change horizontal angle
  {
    if (avl > avr)
    {
    servoh = --servoh;
      if (servoh < servohLimitLow)
      {
        servoh = servohLimitLow;
      }
    } else if (avl < avr)
    {
      servoh = ++servoh;
      if (servoh > servohLimitHigh)
      {
      servoh = servohLimitHigh;
      }
    }
    else if (avl = avr)
    {
      delay(5000);
    }
    horizontal.write(servoh);
  }

  */

}

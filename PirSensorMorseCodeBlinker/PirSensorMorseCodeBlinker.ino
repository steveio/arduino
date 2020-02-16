/*
  PIR Sensor Morse Code Blinker

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

const int pin_led = 9;                 // LED Out pin 
const int pin_pir = 2;                 // PIR In pin 
volatile byte state = LOW;
volatile byte active = LOW;


#define MORSE_NONE 0x01
#define DELAY_DIT 1000
#define DELAY_DAH 3000

/* Each byte is prefixed with a variable number of zero bits and a
 * single 1, followed by the morse code representation of the
 * character where 0 = dit and 1 = dah.  For example, the character
 * e is 0b0000 0010; that is, six prefix 0s, a 1 to indicate the
 * beginning of the character, and a single 0 represinting the dit.
 * The table is in ASCII order, so indexing a character into the
 * table directly yields its morse representation.  The character
 * set is incomplete.  (Notably, some symbols and all prosigns are
 * missing.)  Missing characters cause a dah-length inter-character
 * delay with no intervening symbols. 
 * From: https://kb8ojh.net/msp430/morse_spin.c
 */
const unsigned char morse_ascii[] = {
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    0x73, MORSE_NONE, 0x55, 0x32,                   /* , _ . / */
    0x3F, 0x2F, 0x27, 0x23,                         /* 0 1 2 3 */
    0x21, 0x20, 0x30, 0x38,                         /* 4 5 6 7 */
    0x3C, 0x37, MORSE_NONE, MORSE_NONE,             /* 8 9 _ _ */
    MORSE_NONE, 0x31, MORSE_NONE, 0x4C,             /* _ = _ ? */
    MORSE_NONE, 0x05, 0x18, 0x1A,                   /* _ A B C */
    0x0C, 0x02, 0x12, 0x0E,                         /* D E F G */
    0x10, 0x04, 0x17, 0x0D,                         /* H I J K */
    0x14, 0x07, 0x06, 0x0F,                         /* L M N O */
    0x16, 0x1D, 0x0A, 0x08,                         /* P Q R S */
    0x03, 0x09, 0x11, 0x0B,                         /* T U V W */
    0x19, 0x1B, 0x1C, MORSE_NONE,                   /* X Y Z _ */
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
    MORSE_NONE, 0x05, 0x18, 0x1A,                   /* _ A B C */
    0x0C, 0x02, 0x12, 0x0E,                         /* D E F G */
    0x10, 0x04, 0x17, 0x0D,                         /* H I J K */
    0x14, 0x07, 0x06, 0x0F,                         /* L M N O */
    0x16, 0x1D, 0x0A, 0x08,                         /* P Q R S */
    0x03, 0x09, 0x11, 0x0B,                         /* T U V W */
    0x19, 0x1B, 0x1C, MORSE_NONE,                   /* X Y Z _ */
    MORSE_NONE, MORSE_NONE, MORSE_NONE, MORSE_NONE,
};


// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(19200);

  pinMode(pin_pir, INPUT);
  pinMode(pin_led, OUTPUT);

  Serial.println("Sensor Calibration");

  for(int i = 0; i < 20 ; i++){
   Serial.print(".");
   delay(1000);
  }
  Serial.println("Sensor Calibrated");

  attachInterrupt(digitalPinToInterrupt(pin_pir),interrupt_routine,RISING);
}

void interrupt_routine(){
  state = HIGH;
}

// the loop function runs over and over again forever
void loop() {

  if (state == HIGH && active == LOW)
  {

    active = HIGH;

    Serial.println("Motion Detected");
    Serial.println(digitalRead(pin_pir));

    const char *msg = "foxtrot tango";
    morse_tx(msg);
    
    //Serial.println("low");
    state = LOW;
    digitalWrite(pin_led,LOW);

    active = LOW;
  }
 
}

void morse_tx(const char *msg) {

    int inchar = 0; /* Indicates whether we are in the prefix
                     * padding or the character itself. */
    unsigned int j;

    Serial.println("morse_tx: ");
    Serial.println(msg);

    int i;

    /* Send the string in str as morse code */
    for (i = 0; msg[i]; i++) {
        int inchar = 0; /* Indicates whether we are in the prefix
                         * padding or the character itself. */

        Serial.println(msg[i]);

        unsigned int j;
        for (j = 0; j < 8; j++) {

              
            /* The following lookup yields true for dah and false
             * for dit at each position */
            int bit = morse_ascii[(int)msg[i]] & (0x80 >> j);
            if (inchar) {
               /* Set the output pin, delay for dit or dah
                * period according to bit, then unset the
                * output pin. */
                digitalWrite(pin_led, HIGH);
                delay(bit ? DELAY_DAH : DELAY_DIT);
                digitalWrite(pin_led, LOW);
                Serial.print(bit ? "-" : ".");
                if (j < 7) {
                    /* If we're not yet to the end of the character,
                     * delay one dit period for intra-char delay */
                    delay(DELAY_DIT);
                    Serial.print(" ");
                }
            } else if (bit) {
                /* This is the end of the padding, set inchar
                 * and iterate. */
                inchar = 1;
            }
        }
        delay(DELAY_DAH); /* Inter-character delay. */
        Serial.print("  \n");
    }
    /* Delay two more dahs at the end of the string, before we
     * repeat. */
    delay(DELAY_DAH);
    delay(DELAY_DAH);
}

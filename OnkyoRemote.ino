/*

IR remote for Onkyo A911 amp with C711 CD (missing RC-259S remote)
 16 keys
 power down when idle
 using pin interrupts and watchdog timer

 */
#include <IRremote.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

#define SERIALDEBUG 0

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
// Compatible Onkyo A-911
#define KEY_POWER                0x4BB620DF
#define KEY_SLEEP                0x4BB6BA45
#define SPEAKERS_A               0x4BB69A65
#define SPEAKERS_B               0x4BB65AA5
#define SOURCE_DIRECT            0x4BB622DD
//#define MULTI_SOURCE             0x4BB633CC
#define INPUT_LINE                0x4BB6609F
#define INPUT_TAPE               0x4BB610EF
#define INPUT_TAPE_2             0x4BB6E01F
#define INPUT_TUNER              0x4BB6D02F
#define INPUT_PHONO              0x4BB650AF
#define INPUT_CD                 0x4BB6906F
#define INPUT_VIDEO              0x4BB6F00F
#define INPUT_VIDEO_2            0x4BB6708F
#define INPUT_VIDEO_3            0x4BB6B04F
#define DECK-A_REV-PLAY          0x4BB6F20D
#define DECK-A_FWD-PLAY          0x4BB6728D
#define DECK-A_PAUSE             0x4BB60AF5
#define DECK-A_STOP              0x4BB6B24D
#define DECK-A_REW               0x4BB64AB5
#define DECK-A_FF                0x4BB68A75
#define DECK-B_REV-PLAY          0x4BB66897
#define DECK-B_FWD-PLAY          0x4BB6A857
#define DECK-B_PAUSE             0x4BB618E7
#define DECK-B_STOP              0x4BB6C837
#define DECK-B_REW               0x4BB658A7
#define DECK-B_FF                0x4BB69867
//#define CD_PAUSE                 0x4BB6F807
//#define CD_PLAY                  0x4BB6D827
//#define CD_STOP                  0x4BB638C7
//#define CD_SKIP_PREV             0x4BB67887
//#define CD_SKIP_NEXT             0x4BB6B847
#define TUNER_CLASS-PRESET       0x4BB652AD
#define TUNER_CLASS-PRESET_UP    0x4BB600FF
#define TUNER_CLASS-PRESET_DOWN  0x4BB6807F
#define MUTING                   0x4BB6A05F
#define KEY_VOLUMEUP             0x4BB640BF
#define KEY_VOLUMEDOWN           0x4BB6C03F

// Compatible Onkyo C-711 (no remote cable needed between amp and CD)
#define CD_EJECT                 0x4B34D02F
#define CD_STOP                  0x4B3438C7
#define CD_PLAY                  0x4B34D827
#define CD_REW                   0x4B34807F
#define CD_FWD                   0x4B3400FF
#define CD_PREV_TRACK            0x4B347887
#define CD_NEXT_TRACK            0x4B34B847
#define CD_MEMORY                0x4B34906F
#define CD_CLEAR                 0x4B3410EF
#define CD_SHUFFLE               0x4B34629D
#define CD_PAUSE                 0x4B34F807
#define CD_TRACK_1               0x4B3408F7
#define CD_TRACK_2               0x4B348877
#define CD_TRACK_3               0x4B3448B7
#define CD_TRACK_4               0x4B34C837
#define CD_TRACK_5               0x4B3418E7
#define CD_TRACK_6               0x4B349867
#define CD_TRACK_7               0x4B3458A7
#define CD_TRACK_8               0x4B3430CF
#define CD_TRACK_9               0x4B34B04F
#define CD_TRACK_0               0x4B34708F
#define CD_TRACK_PLUS_10         0x4B34F00F
#define CD_DISPLAY               0x4B3450AF
#define CD_FADE                  0x4B347A85
#define CD_TIME_EDIT             0x4B34B24D
#define CD_AUTO_SPACE            0x4B3432CD
#define CD_VOLUME_UP             0x4B3440BF
#define CD_VOLUME_DOWN           0x4B34C03F
#define CD_REPEAT                0x4B34609F
#define CD_A_B                   0x4B34E01F
#define CD_PEAK_SEARCH           0x4B34BA45

IRsend irsend;
#define send(cmd) irsend.sendNEC(cmd, 32)

#define LED 13

// The remote sleeps between keystrokes. An interrupt awakes the program when a key is pressed.
// Only 2 rows on the keypad because there is only 2 interrupt pins on Arduino Mega or Pro Mini
// Drawback: 10 pins used (8 for a 4x4 keaypad)
#define ROWS 2
#define COLS 8
int row[] = {2, 3}; // Defining row pins of keypad connected to Aeduino pins
int col[] = {4, 5, 6, 7, 8, 10, 11, 12}; //Defining column pins of keypad connected to Arduino Pin 9 for IRled
int i, j;

//volatile int intFlag = 0;
unsigned long cnt = 0;
bool inPlay = false, repeatKey = false;

void setup()
{
#if SERIALDEBUG == 1
  Serial.begin(115200);
  Serial.println("Init...");
#endif
  // CPU Sleep Modes
  // SM2 SM1 SM0 Sleep Mode
  // 0    0  0 Idle
  // 0    0  1 ADC Noise Reduction
  // 0    1  0 Power-down
  // 0    1  1 Power-save
  // 1    0  0 Reserved
  // 1    0  1 Reserved
  // 1    1  0 Standby(1)
  cbi( SMCR, SE );     // sleep enable, power down mode
  cbi( SMCR, SM0 );    // power down mode
  sbi( SMCR, SM1 );    // power down mode
  cbi( SMCR, SM2 );    // power down mode
  cbi(ADCSRA, ADEN);       // switch Analog to Digitalconverter OFF. Less current during sleep

  pinMode(LED, OUTPUT);
}


void loop()
{
  if (++cnt > 5) { // Sleep only after 2.5s idle
    cnt = 0;
    wdt_disable(); // No more timer wakeup
#if SERIALDEBUG == 1
    Serial.println("Sleep"); Serial.flush();
#endif

 
    if (digitalRead(2) && digitalRead(3)) {
    // Setup for interrupt on row pins
    for (i = 0; i < ROWS; i++) {
      pinMode(row[i], INPUT_PULLUP);
//      digitalWrite(row[i], HIGH);
     }
    delay(10);
    for (i = 0; i < ROWS; i++) {
       attachInterrupt(digitalPinToInterrupt(row[i]), pinInterrupt, LOW);
    }

     // Prepare col pins setup for interrupt
      for (i = 0; i < COLS; i++)
      {
        pinMode(col[i], OUTPUT);
        digitalWrite(col[i], LOW);
      }
      enterSleep();
    }
#if SERIALDEBUG == 1
    Serial.println("Wakeup"); Serial.flush();
#endif
  }

  repeatKey = false;
  scanKeypad();
  // sleep 250mS after keypress if not volume up/down and not fwd/rew
  if (!repeatKey) {
    setup_watchdog(WDTO_500MS); // Trigger timer interrupt
    enterSleep();
#if SERIALDEBUG == 1
    Serial.println("wdt wakeup"); Serial.flush();
#endif
  }
}

void scanKeypad() {
  // After wakeup. Setup to scan keypad
  for (i = 0; i < ROWS; i++) {
    pinMode(row[i], OUTPUT);
  }
  for (i = 0; i < COLS; i++)   {
    pinMode(col[i], INPUT);
    digitalWrite(col[i], HIGH);
  }

  // Scan
  for (i = 0; i < ROWS; i++)
  {
    digitalWrite(row[0], HIGH);
    digitalWrite(row[1], HIGH);

    digitalWrite(row[i], LOW);
    for (j = 0; j < COLS; j++)
    {
      if (digitalRead(col[j]) == LOW)
      {
        digitalWrite(LED, HIGH);
        keypress(i, j);
        delay(5);
        digitalWrite(LED, LOW);
      }
    }
  }

}

void keypress(int r, int c)
{
  cnt = 0; // Reset awake delay
  switch (r) {
    case 0:
      switch (c) {
        case 0:
#if SERIALDEBUG == 1
          Serial.println("Pwr");
#endif
          send(KEY_POWER); // PowerOn Onkyo
          break;
        case 1:
#if SERIALDEBUG == 1
          Serial.println("Mute");
#endif
          send(MUTING); // PowerOn Onkyo
          break;
        case 2:
#if SERIALDEBUG == 1
          Serial.println("V+");
#endif
          send(KEY_VOLUMEUP);
          repeatKey = true;
          break;
        case 3:
#if SERIALDEBUG == 1
          Serial.println("V-");
#endif
          send(KEY_VOLUMEDOWN);
          repeatKey = true;
          break;
        case 4:
#if SERIALDEBUG == 1
          Serial.println("Stop");
#endif
          send(CD_STOP);
          inPlay = false;
          break;
        case 5:
#if SERIALDEBUG == 1
          Serial.println("|< prev");
#endif
          send(CD_PREV_TRACK);
          break;
        case 6:
#if SERIALDEBUG == 1
          Serial.println(">| next");
#endif
          send(CD_NEXT_TRACK);
          break;
        case 7:
          if (inPlay) {
#if SERIALDEBUG == 1
            Serial.println("|| pause");
#endif
            send(CD_PAUSE);
            inPlay = false;
          }
          else {
#if SERIALDEBUG == 1
            Serial.println("|> play");
#endif
            send(CD_PLAY);
            inPlay = true;
          }
          break;
      }
      break;
    case 1:
      switch (c) {
        case 0:
#if SERIALDEBUG == 1
          Serial.println("Display");
#endif
          send(CD_DISPLAY);
          break;
        case 1:
#if SERIALDEBUG == 1
          Serial.println("<< rewind");
#endif
          send(CD_REW); // PowerOn Onkyo
          repeatKey = true;
          break;
        case 2:
#if SERIALDEBUG == 1
          Serial.println(">> forward");
#endif
          send(CD_FWD); // PowerOn Onkyo
          repeatKey = true;
          break;
        case 3:
#if SERIALDEBUG == 1
          Serial.println("Eject");
#endif
          send(CD_EJECT);
          break;
        case 4:
#if SERIALDEBUG == 1
          Serial.println("Phono");
#endif
          send(INPUT_PHONO);
          break;
        case 5:
#if SERIALDEBUG == 1
          Serial.println("Tuner");
#endif
          send(INPUT_TUNER);
          break;
        case 6:
#if SERIALDEBUG == 1
          Serial.println("CD");
#endif
          send(INPUT_CD);
          break;
        case 7:
#if SERIALDEBUG == 1
          Serial.println("Line");
#endif
          send(INPUT_LINE);
          break;
      }
      break;
  }
  digitalWrite(LED, LOW);
}

void pinInterrupt(void)
{
  //detachInterrupt(0); Risk of coma
  //detachInterrupt(1);
  for (i = 0; i < ROWS; i++) { // Wakeup. Stop keypad interrupts
    detachInterrupt(digitalPinToInterrupt(row[i]));
  }
}

void enterSleep(void)
{
#if SERIALDEBUG == 1
  //Serial.print("spin2="); Serial.println(digitalRead(2));Serial.flush();
#endif
  pinMode(LED, INPUT); //Less current during sleep...
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  /* The program will continue from here. */
  /* First thing to do is disable sleep. */
  sleep_disable();
  pinMode(LED, OUTPUT); // Restore LED pin setup

}

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
// Without watchdog reset !
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii = 9;
  bb = ii & 7;
  if (ii > 7) bb |= (1 << 5);
  bb |= (1 << WDCE);
  ww = bb;
  // Serial.println(ww);

  MCUSR &= ~(1 << WDRF);
  // start timed sequence
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}
//****************************************************************
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
}


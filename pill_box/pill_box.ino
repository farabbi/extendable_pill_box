/* Extendable Pill Box
 *  by Jiuxin Zhu and Wanyue Wang, Feb 17, 2019
 */

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>
#include "buzzer.h"
#include <Adafruit_NeoPixel.h>

//neopixels setup
#define PIN            24
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      6

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//speaker setup
// notes in the melody:
int melody[] = { 262,196, 196, 220, 196, 0,  247, 262
  /*NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4*/ };

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4,4,4,4,4 };

int current_note = 0;
int current_duration = 0;

const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

RTC_DS3231 rtc;

int screen = 0;
int buzzer_on = 0;
int light_on[6] = {0, 0, 0, 0, 0, 0};

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

class Clock {
  protected:
    int hour_tens = 0;
    int hour_ones = 0;
    int minute_tens = 0;
    int minute_ones = 1;
    int second_tens = 0;
    int second_ones = 0;
    int cursor_pos = 6; // hour tens position X_:_ _:_ _
    int is_setting = 0;
    int is_alarm = 0;
    int is_on = 0;

  public:
    virtual void show_time() = 0;
    virtual void begin_setting() = 0;
    virtual void end_setting() = 0;
    virtual void move_cursor() = 0;
    virtual void inc_digit() = 0;

    int is_Setting() {
      return is_setting;
    }

    int cursor_Pos() {
      return cursor_pos;
    }

    int is_Alarm() {
      return is_alarm;
    }

    int switchStatus() {
      if (!is_setting) {
        if (is_on) {
          is_on = 0;
        } else {
          is_on = 1;
        }
      }
    }
};

class TimeClock : public Clock {
  public:   
    void show_time() {
      if (!is_setting) {
        DateTime now = rtc.now();
        hour_tens = now.hour() / 10;
        hour_ones = now.hour() % 10;
        minute_tens = now.minute() / 10;
        minute_ones = now.minute() % 10;
        second_tens = now.second() / 10;
        second_ones = now.second() % 10;
        lcd.setCursor(0, 0);
        lcd.print("Time: ");
        lcd.print(hour_tens);
        lcd.print(hour_ones);
        lcd.print(':');
        lcd.print(minute_tens);
        lcd.print(minute_ones);
        lcd.print(':');
        lcd.print(second_tens);
        lcd.print(second_ones);
        Serial.print(hour_tens);
        Serial.print(hour_ones);
        Serial.print(':');
        Serial.print(minute_tens);
        Serial.print(minute_ones);
        Serial.print(':');
        Serial.print(second_tens);
        Serial.println(second_ones);
      }
    }

    void begin_setting() {
      cursor_pos = 6;
      lcd.setCursor(cursor_pos, 0);
      lcd.blink();
      is_setting = 1;
    }

    void end_setting() {
      DateTime now = rtc.now();
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour_tens * 10 + hour_ones, minute_tens * 10 + minute_ones, second_tens * 10 + second_ones));
      lcd.noBlink();
      is_setting = 0;
    }

    void move_cursor() {
      switch (cursor_pos) {
        case 6: case 9: case 12:
          cursor_pos++;
          break;
        case 7: case 10:
          cursor_pos += 2;
          break;
        case 13:
          cursor_pos = 6;
          break;
        default:
          break;
      }
      lcd.setCursor(cursor_pos, 0);
    }

    void inc_digit() {
      switch (cursor_pos) {
        case 6:
          hour_tens++;
          if (hour_ones <= 3) {
            hour_tens %= 3;
          } else {
            hour_tens %= 2;
          }
          break;
        case 7:
          hour_ones++;
          if (hour_tens > 1) {
            hour_ones %= 4;
          } else {
            hour_ones %= 10;
          }
          break;
        case 9:
          minute_tens++;
          minute_tens %= 6;
          break;
        case 10:
          minute_ones++;
          minute_ones %= 10;
          break;
        case 12:
          second_tens++;
          second_tens %= 6;
          break;
        case 13:
          second_ones++;
          second_ones %= 10;
          break;
        default:
          break;
      }
      lcd.setCursor(0, 0);
      lcd.print("Time: ");
      lcd.print(hour_tens);
      lcd.print(hour_ones);
      lcd.print(':');
      lcd.print(minute_tens);
      lcd.print(minute_ones);
      lcd.print(':');
      lcd.print(second_tens);
      lcd.print(second_ones);
      lcd.setCursor(cursor_pos, 0);
    }
};

class AlarmClock : public Clock {
  public:
    void show_time() {
      if (!is_setting) {
        lcd.setCursor(0,0);
        lcd.print("Alarm");
        lcd.print(screen);
        lcd.print(": ");
        Serial.print(is_on);
        if (is_on) {
          lcd.print("ON ");
        } else {
          lcd.print("OFF");
        }
        lcd.setCursor(6, 1);
        lcd.print(hour_tens);
        lcd.print(hour_ones);
        lcd.print(':');
        lcd.print(minute_tens);
        lcd.print(minute_ones);
        lcd.print(':');
        lcd.print(second_tens);
        lcd.print(second_ones);
      }
    }

    void begin_setting() {
      cursor_pos = 6;
      lcd.setCursor(cursor_pos, 1);
      lcd.blink();
      is_setting = 1;
    }

    void end_setting() {
      lcd.noBlink();
      is_setting = 0;
    }

    void move_cursor() {
      switch (cursor_pos) {
        case 6: case 9: case 12:
          cursor_pos++;
          break;
        case 7: case 10:
          cursor_pos += 2;
          break;
        case 13:
          cursor_pos = 6;
          break;
        default:
          break;
      }
      lcd.setCursor(cursor_pos, 1);
    }

    void inc_digit() {
      switch (cursor_pos) {
        case 6:
          hour_tens++;
          if (hour_ones <= 3) {
            hour_tens %= 3;
          } else {
            hour_tens %= 2;
          }
          break;
        case 7:
          hour_ones++;
          if (hour_tens > 1) {
            hour_ones %= 4;
          } else {
            hour_ones %= 10;
          }
          break;
        case 9:
          minute_tens++;
          minute_tens %= 6;
          break;
        case 10:
          minute_ones++;
          minute_ones %= 10;
          break;
        case 12:
          second_tens++;
          second_tens %= 6;
          break;
        case 13:
          second_ones++;
          second_ones %= 10;
          break;
        default:
          break;
      }
      lcd.setCursor(0,0);
      lcd.print("Alarm");
      lcd.print(screen);
      lcd.print(": ");
      if (is_on) {
        lcd.print("ON ");
      } else {
        lcd.print("OFF");
      }
      lcd.setCursor(6, 1);
      lcd.print(hour_tens);
      lcd.print(hour_ones);
      lcd.print(':');
      lcd.print(minute_tens);
      lcd.print(minute_ones);
      lcd.print(':');
      lcd.print(second_tens);
      lcd.print(second_ones);
      lcd.setCursor(cursor_pos, 1);
    }

    int is_On() {
      return is_on;
    }

    // same reuturn 1, else return 0
    int check_alarm() {
      DateTime now = rtc.now();
      if ( (hour_tens == now.hour() / 10) &&
           (hour_ones == now.hour() % 10) &&
           (minute_tens == now.minute() / 10) &&
           (minute_ones == now.minute() % 10) &&
           (second_tens == now.second() / 10) &&
           (second_ones == now.second() % 10) ) {
        return 1;
      } else {
        return 0;
      }
    }
};

AlarmClock alarmClock[6];
TimeClock timeClock;
Clock *current_clock = &timeClock;

class Button {
    const byte pin;
    int state;
    unsigned long buttonDownMs;
    int long_press;

  protected:
    virtual void shortClick() = 0;
    virtual void longClick() = 0;
    virtual void switchStatus() = 0;
    virtual void play_melody() = 0;

  public:
    Button(byte attachTo) :
      pin(attachTo) {
    }

    void set() {
      pinMode(pin, INPUT);
      state = LOW;
      long_press = 0;
    }

    void detect() {
      play_melody();
      int prevState = state;
      state = digitalRead(pin);
      Serial.println(state);
      if (prevState == LOW && state == HIGH) {
        buttonDownMs = millis();
        // for alarm clock
        switchStatus();
      }
      else if (prevState == HIGH && state == HIGH && long_press == 0) {
        if (millis() - buttonDownMs >= 1000) {
          // long click
          longClick();
          long_press = 1;
        }
      }
      else if (prevState == HIGH && state == LOW) {
        if (millis() - buttonDownMs < 50) {
          // ignore this for debounce
        }
        else if (long_press == 0) {
          // short click
          shortClick();
        } else {
          // release long press flag
          long_press = 0;
        }
      }
    }
};

class SettingButton : public Button {
  Clock *clk;
  
  public:
    SettingButton(byte attachTo, Clock *attachToClock) :
      Button(attachTo),
      clk(attachToClock) {
    }

    void updateCurrentClock() {
      clk = current_clock;
    }

  protected:
    void shortClick() {
      if (clk->is_Setting()) {
        clk->move_cursor();
      } else {
          screen++;
          screen %= 7;
          if (screen == 0) {
            current_clock = &timeClock;
          } else {
            current_clock = &alarmClock[screen-1];
          }
          lcd.clear(); // clean screen and put cursor at upper-left corner
        
      }
    }

    void longClick() {
      if (clk->is_Setting()) {
        clk->end_setting();
      } else {
        clk->begin_setting();
      }
    }

    void switchStatus() {
    }

    void play_melody() {
    }
};

class IncButton : public Button {
  Clock *clk;
  
  public:
    IncButton(byte attachTo, Clock *attachToClock) :
      Button(attachTo),
      clk(attachToClock) {
    }

    void updateCurrentClock() {
      clk = current_clock;
    }
    
  protected:
    void shortClick() {
      if (clk->is_Setting()) {
        clk->inc_digit();
      } else {
        buzzer_on = 0;
        //neopixel off
        for(int i=0;i<NUMPIXELS;i++){
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(0,0,0)); 
        }
        pixels.show(); // This sends the updated pixel color to the hardware.
      }
    }

    void longClick() {
      if (clk->is_Setting()) {
        clk->inc_digit();
      }
    }

    void switchStatus() {
      current_clock->switchStatus();
    }

    void play_melody() {
      if (buzzer_on) {
        for (int thisNote = 0; thisNote < 8; thisNote++) {

          // to calculate the note duration, take one second 
          // divided by the note type.
          //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
          int noteDuration = 1000/noteDurations[thisNote];
          tone(39, melody[thisNote],noteDuration);

          // to distinguish the notes, set a minimum time between them.
          // the note's duration + 30% seems to work well:
          int pauseBetweenNotes = noteDuration * 1.30;
          delay(pauseBetweenNotes);
          // stop the tone playing:
          noTone(39);
        }
      }
    }
};

SettingButton settingButton(3, current_clock);
IncButton incButton(4, current_clock);

void setup () {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  rtc.begin();

  settingButton.set();
  incButton.set();
  pinMode(39, OUTPUT); // buzzer

  Serial.begin(9600);

  pixels.begin(); // This initializes the NeoPixel library.

  //neopixel off
  for(int i=0;i<NUMPIXELS;i++){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,0,0)); 
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
}

void loop () {
  current_clock->show_time();
  settingButton.detect();
  incButton.detect();
  // tricky!
  settingButton.updateCurrentClock();
  incButton.updateCurrentClock();
  for (int i = 0; i < 6; i++) {
    if (alarmClock[i].is_On()) {
      // same return 1, else return 0
      if (alarmClock[i].check_alarm()) {
        pixels.setPixelColor(i, pixels.Color(0, 150, 0)); // Moderately bright green color.
        pixels.show(); // This sends the updated pixel color to the hardware.
        buzzer_on = 1;
      }
    }
  }
  //alarm_on
//  if (buzzer_on) {
    //buzzer
//    for (int i = 0; i < 6; i++) {
//      pixels.setPixelColor(i, pixels.Color(0, 150, 0)); // Moderately bright green color.
//    }
//    pixels.show(); // This sends the updated pixel color to the hardware.
//    current_duration = 1000/noteDurations[current_note];
//    tone(39, melody[current_note], current_duration);
//    delay(current_duration*1.30);
//    current_note++;
//    current_note %= 8;
//    noTone(39);
    
//    for (int thisNote = 0; thisNote < 8; thisNote++) {

      // to calculate the note duration, take one second 
      // divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
//      int noteDuration = 1000/noteDurations[thisNote];
//      tone(39, melody[thisNote],noteDuration);

      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
//      int pauseBetweenNotes = noteDuration * 1.30;
//      delay(pauseBetweenNotes);
      // stop the tone playing:
      //noTone(39);
//    }
    //delay(3000);
//  } else {
//    noTone(39);
//    //neopixel off
//    for(int i=0;i<NUMPIXELS;i++){
//
//      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
//      pixels.setPixelColor(i, pixels.Color(0,0,0)); 
//      pixels.show(); // This sends the updated pixel color to the hardware.
//
//    }
//    for (int i = 0; i < 6; i++) {
//      light_on[i] = 0;
//    }
//  }
}

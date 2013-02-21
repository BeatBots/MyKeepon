/* My Keepon Arduino controller
   Copyright © 2012 BeatBots LLC

   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   For a copy of the GNU General Public License, see
   http://www.gnu.org/licenses/gpl.html

   For support, please post issues on the Github project page at
   http://github.com/BeatBots/MyKeepon

   Learn more about My Keepon at
   http://mykeepon.beatbots.net

   Keepon® is a trademark of BeatBots LLC.
*/

#include <Wire.h>

#define cbi(sfr, bit) _SFR_BYTE(sfr) &= ~_BV(bit)
#define sbi(sfr, bit) _SFR_BYTE(sfr) |= _BV(bit)

#define MK_FREQ 49600L // Set clock to 50kHz (actualy 49.6kHz seems to work better)
#define SOUND (byte)0x52  // Sound controller (device ID 82).  Write to 0xA4, read from 0xA5.
#define BUTTON (byte)0x50 // Button controller (device ID 80). Write to 0xA0, read from 0xA1.
#define MOTOR (byte)0x55  // Motor controller (device ID 85).  Write to 0xAA, read from 0xAB.

void setup()
{
  pinMode(SDA, OUTPUT); // Data wire on My Keepon
  pinMode(SCL, OUTPUT); // Clock wire on My Keepon
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);
  Serial.begin(115200);
  while(!Serial);
}

void bootup()
{
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);
  Serial.print("Waiting for My Keepon... ");
  while (analogRead(0) < 512); // Wait until we see voltage on A0 pin
  Serial.println("My Keepon detected.");
  delay(1000);
  Wire.begin();
  TWBR = ((F_CPU / MK_FREQ) - 16) / 2;
  Serial.write((byte)0);
}

boolean isNextWord(char* msg, char* wordToCompare, int* i) {
  int j = 0;
  while (msg[j] == wordToCompare[j++]);
  if (j >= strlen(wordToCompare)) {
    *i = *i+j;
    return 1;
  } 
  else return 0;
}

int nextInt(char* msg) {
  int j = 0;
  int value = 0;
  int negative = 0;
  while (!(isDigit(msg[j]) || msg[j]=='-')) j++;
  if (msg[j] == '-') {
    negative = 1;
    j++;
  }
  while (isDigit(msg[j])) {
    value *= 10;
    value += msg[j++]-'0';
  }
  if (negative) value *= -1;
  return value;
}

boolean parseMsg(char* msg, byte* cmd, byte* device) {
  int i = 0, value;

  if (isNextWord(&msg[i], "SOUND", &i)) {
    *device = SOUND;
    if (isNextWord(&msg[i], "PLAY", &i)) {
      cmd[0] = 0x01;
      cmd[1] = B10000000 | (63 & nextInt(&msg[i]));
    } 
    else if (isNextWord(&msg[i], "REPEAT", &i)) {
      cmd[0] = 0x01;
      cmd[1] = B11000000 | (63 & nextInt(&msg[i]));
    } 
    else if (isNextWord(&msg[i], "DELAY", &i)) {
      cmd[0] = 0x03;
      cmd[1] = (byte)nextInt(&msg[i]);
    } 
    else if (isNextWord(&msg[i], "STOP", &i)) {
      cmd[0] = 0x01;
      cmd[1] = B00000000;
    } 
    else {
      Serial.println("Unknown command.");
      return false;
    }
  } 
  else if (isNextWord(&msg[i], "SPEED", &i)) {
    *device = MOTOR;
    if (isNextWord(&msg[i], "PAN", &i)) {
      cmd[0] = 5;
      cmd[1] = (byte)nextInt(&msg[i]);
    } 
    else if (isNextWord(&msg[i], "TILT", &i)) {
      cmd[0] = 3;
      cmd[1] = (byte)nextInt(&msg[i]);
    } 
    else if (isNextWord(&msg[i], "PONSIDE", &i)) {
      cmd[0] = 1;
      cmd[1] = (byte)nextInt(&msg[i]);
    } 
    else {
      Serial.println("Unknown command.");
      return false;
    }
  } 
  else if (isNextWord(&msg[i], "MOVE", &i)) {
    *device = MOTOR;
    if (isNextWord(&msg[i], "PAN", &i)) {
      cmd[0] = 4;
      cmd[1] = (byte)(nextInt(&msg[i]) + 127);
    } 
    else if (isNextWord(&msg[i], "TILT", &i)) {
      cmd[0] = 2;
      cmd[1] = (byte)(nextInt(&msg[i]) + 127);
    } 
    else if (isNextWord(&msg[i], "SIDE", &i)) {
      cmd[0] = 0;
      if (isNextWord(&msg[i], "CYCLE", &i))
        cmd[1] = 0;
      else if (isNextWord(&msg[i], "CENTERFROMLEFT", &i))
        cmd[1] = 1;
      else if (isNextWord(&msg[i], "RIGHT", &i))
        cmd[1] = 2;
      else if (isNextWord(&msg[i], "CENTERFROMRIGHT", &i))
        cmd[1] = 3;
      else if (isNextWord(&msg[i], "LEFT", &i))
        cmd[1] = 4;
      else {
        Serial.println("Unknown command.");
        return false;
      }
    } 
    else if (isNextWord(&msg[i], "PON", &i)) {
      cmd[0] = 0;
      if (isNextWord(&msg[i], "UP", &i))
        cmd[1] = -1;
      else if (isNextWord(&msg[i], "HALFDOWN", &i))
        cmd[1] = -2;
      else if (isNextWord(&msg[i], "DOWN", &i))
        cmd[1] = -3;
      else if (isNextWord(&msg[i], "HALFUP", &i))
        cmd[1] = -4;
      else {
        Serial.println("Unknown command.");
        return false;
      }
    } 
    else if (isNextWord(&msg[i], "STOP", &i)) {
      cmd[0] = 6;
      cmd[1] = 16;
    } 
    else {
      Serial.println("Unknown command.");
      return false;
    }    
  } 
  else if (isNextWord(&msg[i], "MODE", &i)) {
    if (isNextWord(&msg[i], "DANCE", &i)) {
      cmd[0] = 6;
      cmd[1] = 0;
    } 
    else if (isNextWord(&msg[i], "TOUCH", &i)) {
      cmd[0] = 6;
      cmd[1] = 1;
    } 
    else if (isNextWord(&msg[i], "TEMPO", &i)) {
      cmd[0] = 6;
      cmd[1] = 2;
    } 
    else if (isNextWord(&msg[i], "SLEEP", &i)) {
      cmd[0] = 6;
      cmd[1] = 240;
    } 
    else {
      Serial.println("Unknown command.");
      return false;
    }
  } 
  else {
    Serial.println("Unknown command.");
    return false;
  }
  return true;
}

boolean buttonState[8];
char* buttonName[] = {
  "DANCE", "", "HEAD", "TOUCH",
  "RIGHT", "FRONT", "LEFT", "BACK"};

boolean motorState[8];
char* motorName[] = {
  "PON FINISHED", "SIDE FINISHED", "TILT FINISHED", "PAN FINISHED",
  "PON STALLED", "SIDE STALLED", "TILT STALLED", "PAN STALLED"};

int encoderState[4], audioState[5], emfState[3], positionState[3];
char* encoderName[] = {
  "TILT NOREACH", "TILT FORWARD", "TILT BACK", "TILT UP",
  "PON HALFDOWN", "PON UP", "PON DOWN", "PON HALFUP",
  "SIDE CENTER", "SIDE LEFT", "SIDE RIGHT", "SIDE CENTER",
  "PAN BACK", "PAN RIGHT", "PAN LEFT", "PAN CENTER"};

unsigned long updatedButton = 0, updatedMotor = 0;
void query() {
  int i;
  byte buttonResponse, motorResponse;
  int intResponse;

  if (millis() - updatedButton > 100) {
    updatedButton = millis();
    Wire.requestFrom((int)BUTTON, 1);
    if (Wire.available() >= 1) {
      buttonResponse = Wire.read();
      for (i = 0; i < 8; i++) {
        if (i != 1) {
          if (buttonResponse & (1<<i)) {
            if (!buttonState[i]) {
              Serial.print("BUTTON ");
              Serial.print(buttonName[i]);
              Serial.println(" ON");
              buttonState[i] = 1;
            }
          }
          else if (buttonState[i]) {
            Serial.print("BUTTON ");
            Serial.print(buttonName[i]);
            Serial.println(" OFF");
            buttonState[i] = 0;
          }
        }
      }
    }
  }

  if (millis() - updatedMotor > 300) {
    updatedMotor = millis();
    Wire.requestFrom((int)MOTOR, 13);
    if (Wire.available() >= 13) {
      motorResponse = Wire.read();
      for (i = 0; i < 8; i++) {
        if (motorResponse & (1<<i)) {
          if (!motorState[i]) {
            Serial.print("MOTOR ");
            Serial.println(motorName[i]);
            motorState[i] = 1;
          }
        } 
        else if (motorState[i]) {
          motorState[i] = 0;
        }
      }
      motorResponse = Wire.read();
      if (motorResponse != audioState[0]) {
        Serial.print("AUDIO TEMPO ");
        Serial.println(motorResponse);
        audioState[0] = motorResponse;
      }
      motorResponse = Wire.read();
      if (motorResponse != audioState[1]) {
        Serial.print("AUDIO MEAN ");
        Serial.println(motorResponse);
        audioState[1] = motorResponse;
      }
      motorResponse = Wire.read();
      if (motorResponse != audioState[2]) {
        Serial.print("AUDIO RANGE ");
        Serial.println(motorResponse);
        audioState[2] = motorResponse;
      }
      motorResponse = Wire.read();
      for (i = 0; i < 4; i++) {
        if ((motorResponse & (3<<(2*i))) != encoderState[i]) {
          encoderState[i] = motorResponse & (3<<(2*i));
          Serial.print("ENCODER ");
          Serial.println(encoderName[4*i+(encoderState[i]>>(2*i))]);
        }
      }
      motorResponse = Wire.read();
      intResponse = motorResponse;
      if (intResponse > 0) intResponse -= 127;
      if (intResponse != emfState[0]) {
        Serial.print("EMF PONSIDE ");
        Serial.println(intResponse);
        emfState[0] = intResponse;
      }
      motorResponse = Wire.read();
      intResponse = motorResponse;
      if (intResponse > 0) intResponse -= 127;
      if (intResponse != emfState[1]) {
        Serial.print("EMF TILT ");
        Serial.println(intResponse);
        emfState[1] = intResponse;
      }
      motorResponse = Wire.read();
      intResponse = motorResponse;
      if (intResponse > 0) intResponse -= 127;
      if (intResponse != emfState[2]) {
        Serial.print("EMF PAN ");
        Serial.println(intResponse);
        emfState[2] = intResponse;
      }
      motorResponse = Wire.read();
      if (motorResponse != audioState[3]) {
        //      Serial.print("AUDIO ENVELOPE ");
        //      Serial.println(motorResponse);
        audioState[3] = motorResponse;
      }
      motorResponse = Wire.read();
      if (motorResponse != audioState[4]) {
        Serial.print("AUDIO BPM ");
        Serial.println(motorResponse);
        audioState[4] = motorResponse;
      }
      motorResponse = Wire.read();
      intResponse = motorResponse - 127;
      if (intResponse != positionState[0]) {
        Serial.print("POSITION PONSIDE ");
        Serial.println(intResponse);
        positionState[0] = intResponse;
      }
      motorResponse = Wire.read();
      intResponse = motorResponse - 127;
      if (intResponse != positionState[1]) {
        Serial.print("POSITION TILT ");
        Serial.println(intResponse);
        positionState[1] = intResponse;
      }
      motorResponse = Wire.read();
      intResponse = motorResponse - 127;
      if (intResponse != positionState[2]) {
        Serial.print("POSITION PAN ");
        Serial.println(intResponse);
        positionState[2] = intResponse;
      }
    }
  }
}

void loop() {
  char msg[32];
  byte device, cmd[2];

  bootup();

  while (analogRead(0) > 512) {
    query();

    if (Serial.available() > 0) {
      int i = 0;
      while ((msg[i++] = Serial.read()) != ';' && i < 30 && analogRead(0) > 512) {
        while (Serial.available() <= 0 && analogRead(0) > 512);
      }
      msg[i] = '\0';
      if (parseMsg(msg, cmd, &device)) {
        int result = 1;
        int attempts = 0;
        while (result != 0 && attempts++ < 50) {
          Wire.beginTransmission(device);
          Wire.write((byte)cmd[0]);
          Wire.write((byte)cmd[1]);
          result = (int)Wire.endTransmission();
        }
      }
    }
  }
}



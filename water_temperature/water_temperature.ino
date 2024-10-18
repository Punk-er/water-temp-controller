#include <LiquidCrystal.h>
#include <EEPROM.h>
LiquidCrystal lcd(12, 11, 9, 8, 7, 6);
unsigned long int a, time_break, idle;
double speed;
short temp_telorance, temp_set, temp;
int stage = 0, fan_mode = 2, pump_mode = 2;
bool power_on = false, fan_stat, pump_stat, pump_erorr = false;
// int flow_time_error;
// short flow_set;
#define pump 10
#define fan 5
#define TEC 3
class Button {
public:
  unsigned long int firstTime = 0, lastTime = 0;
  bool stat = false, firstpress = true, stillOnHold = false;
  int tapcounter = 0, holdTime = 800, tapTime = 20, tapGap = 400, pin;
  Button(int pin) {
    this->pin = pin;
  }
  void Listen(void (*callback_hold)(int), void (*callback_tap)(int)) {
    // Serial.println(tapcounter);
    if (stat != digitalRead(pin)) {
      stat = digitalRead(pin);
      if (stat)
        firstTime = millis();
      else {
        if (millis() - firstTime > tapTime && millis() - firstTime < holdTime) {
          tapcounter++;
          lastTime = millis();
        }
        if (stillOnHold && millis() - firstTime >= holdTime) {
          stillOnHold = false;
        }
      }
    } else {
      if (!stillOnHold && stat && millis() - firstTime >= holdTime) {
        callback_hold(millis() - firstTime);
        stillOnHold = true;
      }
      if (tapcounter != 0 and millis() - lastTime > tapGap) {
        callback_tap(tapcounter);
        tapcounter = 0;
      }
    }
  }
};
void OnTap(int count) {
  Serial.print("tap: ");
  Serial.println(count);
}
void OnHold(int duriation) {
  Serial.print("hold: ");
  Serial.println(duriation);
}
Button Decr(A4);
Button set(A5);
Button Incr(A6);

void decrease_hold(int dur) {
  if (digitalRead(A6)) {
    power_on = !power_on;
    wEEPROM(16, (String)power_on);

    if (!power_on) {
      digitalWrite(fan, LOW);
      digitalWrite(TEC, LOW);
      digitalWrite(pump, LOW);
      fan_stat = false;
      pump_stat = false;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("OFF");
    } else {
      pump_erorr = false;
      time_break = millis();
      digitalWrite(pump, HIGH);
      pump_stat = true;
    }
  }
}
// void increase_hold(int dur) {
//   if (digitalRead(A4)) {
//     power_on = !power_on;
//     if (!power_on) {
//       digitalWrite(5, LOW);
//       digitalWrite(10, LOW);
//       lcd.clear();
//       lcd.setCursor(6, 0);
//       lcd.print("OFF");
//     }
//   }
// }
void decrease(int count) {
  idle = millis();

  switch (stage) {
    case 1:
      temp_set -= count;
      if (temp_set < 10) temp_set = 10;
      wEEPROM(0, (String)temp_set);
      break;
    case 2:
      temp_telorance -= count;
      if (temp_telorance < 2) temp_telorance = 2;
      wEEPROM(4, (String)temp_telorance);
      break;
    case 3:
      //   flow_set -= 10 * count;
      //   if (temp_telorance < 1) temp_telorance = 1;
      //   wEEPROM(8, (String)flow_set);
      //   break;
      // case 4:
      //   flow_time_error -= 5 * count;
      //   if (flow_time_error < 5) flow_time_error = 5;
      //   wEEPROM(12, (String)flow_time_error);
      //   break;
      // case 5:
      pump_mode++;
      if (pump_mode > 1) pump_mode = 0;
      wEEPROM(24, (String)pump_mode);
      pump_erorr = false;
      time_break = millis();
      digitalWrite(pump, HIGH);
      pump_stat = true;
      break;
  }
}
void increase(int count) {
  idle = millis();

  switch (stage) {
    case 1:
      temp_set += count;
      wEEPROM(0, (String)temp_set);
      break;
    case 2:
      temp_telorance += count;
      wEEPROM(4, (String)temp_telorance);
      break;
    case 3:
      //   flow_set += 10 * count;
      //   wEEPROM(8, (String)flow_set);
      //   break;
      // case 4:
      //   flow_time_error += 5 * count;
      //   wEEPROM(12, (String)flow_time_error);
      //   break;
      // case 5:
      fan_mode++;
      if (fan_mode > 2) fan_mode = 0;
      wEEPROM(20, (String)fan_mode);
      break;
  }
}

void setHold(int dur) {
  if (stage == 0) stage = 1;
  else stage = 0;
}
void setPress(int c) {
  if (pump_erorr) {
    pump_erorr = !pump_erorr;
    time_break = millis();
    digitalWrite(pump, HIGH);
    pump_stat = true;
  } else if (stage > 0) {
    idle = millis();
    stage++;
    if (stage > 3) stage = 1;
  }
}

// void setTap(int count) {
//   if (stage > 0 and count == 0) {
//   }
// }

String one_precision(short temp) {
  String upper = (String)(temp / 10);
  String lower = (String)(temp % 10);
  return upper + "." + lower;
}
void setup() {


  Serial.begin(115200);
  // attachInterrupt(digitalPinToInterrupt(3), WATER, CHANGE);
  pinMode(A2, INPUT);
  pinMode(TEC, OUTPUT);
  lcd.begin(16, 2);
  lcd.clear();
  temp_set = rEEPROM(0).toInt();
  temp_telorance = rEEPROM(4).toInt();
  // flow_set = rEEPROM(8).toInt();
  // flow_time_error = rEEPROM(12).toInt();
  power_on = (bool)rEEPROM(16).toInt();
  fan_mode = rEEPROM(20).toInt();
  pump_mode = rEEPROM(24).toInt();
  digitalWrite(pump, power_on);
  pump_stat = power_on;
  time_break = millis();
}

void loop() {
  // Serial.println("aaaa");
  Decr.Listen(&decrease_hold, &decrease);
  Incr.Listen([](int a) {}, &increase);
  set.Listen(&setHold, &setPress);
  if (power_on) {
    if (fan_mode == 2) {

      temp = (short)(analogRead(A2) * 0.499 * 10);
      Serial.println(temp);
      Serial.println(temp_set);
      Serial.println(temp_telorance);

      if (temp > temp_set + temp_telorance) {
        digitalWrite(TEC, HIGH);
        digitalWrite(fan, HIGH);
        fan_stat = true;
      } else if (temp < temp_set - temp_telorance) {
        digitalWrite(fan, LOW);
        digitalWrite(TEC, LOW);
        fan_stat = false;
      }
    } else {
      digitalWrite(TEC, fan_mode);
      digitalWrite(fan, fan_mode);
      fan_stat = fan_mode;
    }
    digitalWrite(pump, pump_mode);
    pump_stat = pump_mode;

    if (millis() - idle > 10000 && stage > 0) {
      stage = 0;
    } else if (stage == 0) {
      idle = millis();
    }
    menu();
    unsigned long int t = millis();
    while (millis() - t < 800) {
      Decr.Listen(&decrease_hold, &decrease);
      Incr.Listen([](int a) {}, &increase);
      set.Listen(&setHold, &setPress);
    }
    lcd.clear();
  } else {

    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("OFF");
    unsigned long int t = millis();
    while (millis() - t < 800) {
      Decr.Listen(&decrease_hold, &decrease);
      Incr.Listen([](int a) {}, &increase);
      set.Listen(&setHold, &setPress);
    }
  }
}
void menu() {
  switch (stage) {
    case 0:
      // if (!pump_erorr || pump_mode != 2) {
      lcd.setCursor(0, 0);
      lcd.print("Temp:");
      lcd.setCursor(6, 0);
      if (fan_mode != 2) lcd.print("Fors");
      else lcd.print(one_precision(temp));

      if (fan_stat) {
        lcd.setCursor(11, 0);
        lcd.print("F:ON");
      } else {
        lcd.setCursor(11, 0);
        lcd.print("F:OFF");
      }
      // lcd.setCursor(0, 1);
      // lcd.print("Flow:");
      // if (pump_mode != 2) {
      //   lcd.setCursor(6, 1);
      //   lcd.print("Fors");
      // } else {
      //   lcd.setCursor(7, 1);
      //   lcd.print((int)speed);
      // }

      lcd.setCursor(0, 1);
      if (pump_stat) {
        lcd.print("Pump: ON");
      } else {
        lcd.print("Pump: OFF");
      }
      // speed = 0;
      // } else {
      //   lcd.clear();
      //   lcd.setCursor(2, 0);
      //   lcd.print("Water Erorr");
      //   lcd.setCursor(0, 1);
      //   lcd.print("press set: retry");
      // }
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("set temp target");
      lcd.setCursor(2, 1);
      lcd.print("-");
      lcd.setCursor(13, 1);
      lcd.print("+");
      lcd.setCursor(6, 1);
      lcd.print(one_precision(temp_set));
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("temp tolerance");
      lcd.setCursor(2, 1);
      lcd.print("-");
      lcd.setCursor(13, 1);
      lcd.print("+");
      lcd.setCursor(6, 1);
      lcd.print(one_precision(temp_telorance));
      break;
    case 3:
      //   lcd.setCursor(0, 0);
      //   lcd.print("flow threshold");
      //   lcd.setCursor(2, 1);
      //   lcd.print("-");
      //   lcd.setCursor(13, 1);
      //   lcd.print("+");
      //   lcd.setCursor(6, 1);
      //   lcd.print(one_precision(flow_set));
      //   break;
      // case 4:
      //   lcd.setCursor(0, 0);
      //   lcd.print("flow time Error");
      //   lcd.setCursor(2, 1);
      //   lcd.print("-");
      //   lcd.setCursor(13, 1);
      //   lcd.print("+");
      //   lcd.setCursor(6, 1);
      //   lcd.print((String)flow_time_error + " S");
      //   break;
      // case 5:
      lcd.setCursor(2, 0);
      lcd.print("set by force");
      lcd.setCursor(0, 1);
      lcd.print("PmP:");
      lcd.setCursor(4, 1);
      switch (pump_mode) {
        case 0:
          lcd.print("OFF");
          break;
        case 1:
          lcd.print("ON");
          break;
          // case 2:
          //   lcd.print("AUT");
          //   break;
      }
      lcd.setCursor(8, 1);
      lcd.print("Fan:");
      lcd.setCursor(12, 1);
      switch (fan_mode) {
        case 0:
          lcd.print("OFF");
          break;
        case 1:
          lcd.print("ON");
          break;
        case 2:
          lcd.print("AUT");
          break;
      }
      break;
  }
}
// void WATER() {
//   // Serial.println(micros()-a);
//   speed = 900000 / (micros() - a);
//   Serial.println(speed);
//   a = micros();
// }
void wEEPROM(int addrOffset, const String &strToWrite) {
  int len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}
String rEEPROM(int addrOffset) {
  int newStrLen = int(EEPROM.read(addrOffset));
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++) {
    data[i] = char(EEPROM.read(addrOffset + 1 + i));
  }
  data[newStrLen] = '\0';  // !!! NOTE !!! Remove the space between the slash "/" and "0" (I've added a space because otherwise there is a display bug)
  return String(data);
}

#pragma once
#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

class ControlEnc {

  public:
    ControlEnc(byte pinS1, byte pinS2, byte pinKey);
    void initTimer();
    byte getCommand();

  private:
    byte _encID;
};

byte countEnc = 0;
byte *pins = malloc(0);
byte *history = malloc(0);
byte *command = malloc(0);

ControlEnc::ControlEnc(byte pinS1, byte pinS2, byte pinKey) {
  _encID = countEnc;
  countEnc += 1;
  pins = realloc(pins, 3*countEnc);
  history = realloc(history, 5*countEnc);
  command = realloc(command, countEnc);
  pins[(_encID*3)] = pinS1;
  pins[(_encID*3)+1] = pinS2;
  pins[(_encID*3)+2] = pinKey;
  if (countEnc == 1) {
    cli();
    TCCR5A = 0;
    TCCR5B = 0;
    //TCCR5C = 0;
    TIMSK5 |= (1 << TOIE5);  // по совпадению таймера
    TCCR5B |= (1 << CS10);   // без делителя
    sei();
  }
}

byte ControlEnc::getCommand() {
  byte output = command[_encID];
  command[_encID] = 0;
  return output;
}


//==================================================================
//Список команд:
byte turnLeft[] = {7, 5, 4, 6, 7};
byte turnRight[] = {7, 6, 4, 5, 7};
byte pressTurnLeft[] = {3, 1, 0, 2, 3};
byte pressTurnRight[] = {3, 2, 0, 1, 3};
byte press[] = {7, 3, 7};

//Прерывание 5 таймера (20мкс - пассивно, 40мкс - максимум)
ISR(TIMER5_OVF_vect) {
  for (byte encID = 0; encID < countEnc; encID++) {
    byte position = ((digitalRead(pins[(encID * 3) + 2]) << 2) | (digitalRead(pins[(encID * 3)]) << 1) | digitalRead(pins[(encID * 3) + 1]));
    if (history[encID * 5] != position) {

      //Запись позиции в историю
      for (byte index = 4; index > 0; index--) {
        history[(encID * 5) + index] = history[(encID * 5) + (index - 1)];
      }
      history[(encID * 5)] = position;

      //Вычисление команд по истории (1 сравнение = 4мкс)
      command[encID] =
        !memcmp(turnLeft, history, 5) ? 1 :
        !memcmp(turnRight, history, 5) ? 2 :
        !memcmp(pressTurnLeft, history, 5) ? 3 :
        !memcmp(pressTurnRight, history, 5) ? 4 :
        !memcmp(press, history, 3) ? 5 : command[encID];
    }
  }
}

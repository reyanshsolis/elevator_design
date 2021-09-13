
// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// To Set Debug Print On (1) / Off (0)
#define DEBUG 0

// Set the total num of floors.
#define TOTAL_FLOORS 4
// Initialize with Button Pin.
const int buttonPinFloor[TOTAL_FLOORS] = {6, 7, 8, 9};
// Initialize with `current floor` indicating LED.
const int LEDPinFloor[TOTAL_FLOORS] = {0, 1, 10, 13};
// Stores the requests from floors.
bool request_array[TOTAL_FLOORS];

// Stores the current floor.
int current_floor_ = 0;
// Stores the last floor.
int prev_floor_ = 0;

// Stores the time at which the lift was at last floor.
// Since there is no position feedback from lift, a timer
// is used to check if the lift has reached it's
// destination.
unsigned long time_last_floor_;

// Assumed time taken by the lift to travel 1 floor (up or down).
const int floor_to_floor_time = 3000;

// Enum is not supported in Arduino, so using define.
// Stores the state of the lift.
#define STOP 0
#define UP 1
#define DOWN 2
#define GATE_OPEN 3

int state = STOP;

void setup()
{
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Initiating Program");
  delay(50);
  // Initialize request array.
  for (int i = 0; i < TOTAL_FLOORS; ++i)
  {
    pinMode(buttonPinFloor[i], INPUT);
    pinMode(LEDPinFloor[i], OUTPUT);
    request_array[i] = false;
  }

  current_floor_ = 0;
  digitalWrite(LEDPinFloor[current_floor_], HIGH);

  prev_floor_ = 0;
  time_last_floor_ = 0;
  state = STOP;

  lcd.clear();
}


void loop() {

  if (state == STOP) {
    delay(1000);
    time_last_floor_ = millis();
  }

  UpdateRequestArrray(request_array);

  int target_floor = -1, offset = -1;
  GetNextTargetInDirection(request_array, current_floor_, state, target_floor, offset);

  if (millis() - time_last_floor_ > floor_to_floor_time) {
    if (state == UP) {
      current_floor_ = current_floor_ + 1;
    } else if (state == DOWN) {
      current_floor_ = current_floor_ - 1;
    }

    digitalWrite(LEDPinFloor[current_floor_], HIGH);
    digitalWrite(LEDPinFloor[prev_floor_], LOW);
    delay(50);

    prev_floor_ = current_floor_;
    time_last_floor_ = millis();
  }

  if (target_floor != -1) {
    if (current_floor_ == target_floor) {
      request_array[target_floor] = false;

      state = STOP;
      delay(100);
      state = GATE_OPEN;
      LCDPrintInfo(current_floor_, state, target_floor);
      delay(3000);

      if (DEBUG) {
        Serial.print("Reached Floor ");
        Serial.print(target_floor);
        Serial.println(" ");
      }

      state = STOP;

      time_last_floor_ = millis();
    } else {
      // Keep moving in the dir.
      delay(1000);
    }
  }

  LCDPrintInfo(current_floor_, state, target_floor);
  delay(100);
}

void LCDPrintInfo(int current_floor, int state, int next_floor) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(current_floor);
  lcd.print(" ");

  if (state == STOP) {
    lcd.print("DOOR CLOSED ");
  } else if (state == UP) {
    lcd.print("GOING UP    ");
  } else if (state == DOWN) {
    lcd.print("GOING DOWN  ");
  } else {
    lcd.print("DOOR OPEN  ");
  }

  if (next_floor != -1) {
    lcd.print(next_floor);
  }
}


void UpdateRequestArrray(bool* request_array) {

  if (DEBUG) Serial.print("Request Floors: ");

  for (int i = 0; i < TOTAL_FLOORS; ++i) {
    if (digitalRead(buttonPinFloor[i]) == 1) {
      request_array[i] = true;
    }

    if (DEBUG && request_array[i]) {
      Serial.print(" ");
      Serial.print(i);
    }
  }
  if (DEBUG) Serial.println(" ");
}

int GetFirstElementInRequestArrayInDirection(bool *request_array,
    const int &state,
    const int &current_floor) {
  if (state == UP) {
    for (int i = current_floor; i < TOTAL_FLOORS; ++i) {
      if (request_array[i])
        return i;
    }
  }
  else if (state == DOWN) {
    for (int i = current_floor; i >= 0; --i) {
      if (request_array[i])
        return i;
    }
  }

  // If the current floor >= TOTAL_FLOOR - 1 && state == UP
  // or current floor == 0 && state == DOWN
  // or state == STOP
  // return -1.
  return -1;
}

int abs_(const int x) {
  return (x > 0 ? x : -x);
}

void GetNextTargetInDirection(bool *request_array,
                              int &current_floor,
                              int &state,
                              int &target_floor,
                              int &offset) {
  if (state == STOP) {
    int closest_floor_up =
      GetFirstElementInRequestArrayInDirection(
        request_array, UP, current_floor);
    int closest_floor_down =
      GetFirstElementInRequestArrayInDirection(
        request_array, DOWN, current_floor);

    if (closest_floor_up != -1 && closest_floor_down != -1) {
      if ((closest_floor_up - current_floor)
          < (current_floor - closest_floor_down)) {
        target_floor = closest_floor_up;
        state = UP;
        offset = closest_floor_up - current_floor;

        return;
      }
      else {
        target_floor = closest_floor_down;
        state = DOWN;
        offset = current_floor - closest_floor_down;

        return;
      }
    }
    else if (closest_floor_up != -1) {
      target_floor = closest_floor_up;
      state = UP;
      offset = closest_floor_up - current_floor;

      return;
    }
    else if (closest_floor_down != -1) {
      target_floor = closest_floor_down;
      state = DOWN;
      offset = current_floor - closest_floor_down;

      return;
    }
    else {
      target_floor = -1;
      state = STOP;
      offset = -1;

      return;
    }
  }
  else if (state == UP) {
    int closest_floor_up =
      GetFirstElementInRequestArrayInDirection(
        request_array, UP, current_floor);

    if (closest_floor_up != -1) {
      target_floor = closest_floor_up;
      state = UP;
      offset = closest_floor_up - current_floor;

      return;
    }
    else {
      target_floor = -1;
      state = STOP;
      offset = -1;

      return;
    }
  }
  else if (state == DOWN) {
    int closest_floor_down =
      GetFirstElementInRequestArrayInDirection(
        request_array, DOWN, current_floor);

    if (closest_floor_down != -1) {
      target_floor = closest_floor_down;
      state = DOWN;
      offset = current_floor - closest_floor_down;

      return;
    }
    else {
      target_floor = -1;
      state = STOP;
      offset = -1;

      return;
    }
  }

  return;
}


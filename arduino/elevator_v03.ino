
// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// To Set Debug Print On (1) / Off (0)
#define DEBUG 1

// Set the total num of floors.
#define TOTAL_FLOORS 4
// Initialize with Button Pin.
const int FloorRequestUpPin[TOTAL_FLOORS - 1] = {6, 7, 8};
const int FloorRequestDownPin[TOTAL_FLOORS - 1] = {9, 10, 13};

const int LiftRequestPin[TOTAL_FLOORS] = {A0, A1, A2, A3};

const int DoorEmergencyPin = 9;
const int FirePin = A0;

// Stores the requests from floors.
bool request_array[TOTAL_FLOORS];
int request_count = 0;
int buffer_request_count_up = 0;
int buffer_request_count_down = 0;

bool buffer_request_up[TOTAL_FLOORS];
bool buffer_request_down[TOTAL_FLOORS];

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
const int delay_floor_to_floor = 2000;
const int delay_wait_on_floor = 5000;
const int delay_emergency_open = 1000;

// Enum is not supported in Arduino, so using define.
// Stores the direction state of the lift.
#define STOP 0
#define UP 1
#define DOWN 2
int dir_state = STOP;

// Stores the door state of the lift.
#define GATE_OPEN 0
#define GATE_CLOSE 1
int door_state = GATE_CLOSE;

void setup()
{
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Initiating Program");
  delay(50);

  pinMode(DoorEmergencyPin, INPUT);
  pinMode(FirePin, INPUT);

  // Initialize request array.
  for (int i = 0; i < TOTAL_FLOORS; ++i) {
    pinMode(LiftRequestPin[i], INPUT);
    request_array[i] = false;

    if (i != TOTAL_FLOORS - 1) {
      pinMode(FloorRequestUpPin[i], INPUT);
      buffer_request_up[i] = false;
      pinMode(FloorRequestDownPin[i], INPUT);
      buffer_request_down[i] = false;
    }
  }

  request_count = 0;
  buffer_request_count_up = 0;
  buffer_request_count_down = 0;

  current_floor_ = 0;
  prev_floor_ = 0;

  time_last_floor_ = 0;

  dir_state = STOP;
  door_state = GATE_CLOSE;

  lcd.clear();
}


void loop() {

  if (dir_state == STOP) {
    delay(1000);
    time_last_floor_ = millis();
  }

  UpdateFloorRequestArrays(buffer_request_up, buffer_request_down);
  UpdateRequestArrray(request_array, dir_state, buffer_request_up, buffer_request_down);

  int target_floor = -1, offset = -1;
  GetNextTargetInDirection(request_array, current_floor_, dir_state, target_floor, offset);

  if (millis() - time_last_floor_ > delay_floor_to_floor) {
    if (dir_state == UP) {
      current_floor_ = current_floor_ + 1;
    } else if (dir_state == DOWN) {
      current_floor_ = current_floor_ - 1;
    }
    delay(50);

    prev_floor_ = current_floor_;
    time_last_floor_ = millis();
  }

  if (target_floor != -1) {
    if (current_floor_ == target_floor) {
      request_array[target_floor] = false;

      if (buffer_request_up[target_floor]) {
        buffer_request_up[target_floor] = false;
        buffer_request_count_up = buffer_request_count_up - 1;
      }

      if (buffer_request_down[target_floor]) {
        buffer_request_down[target_floor] = false;
        buffer_request_count_down = buffer_request_count_down - 1;
      }

      request_count = request_count - 1;
      if (request_count == 0) dir_state = STOP;

      door_state = GATE_OPEN;
      LCDPrintInfo(current_floor_, dir_state,
                   door_state, target_floor);

      for (int i = 0; i < 10; ++i) {
        UpdateFloorRequestArrays(buffer_request_up, buffer_request_down);
        UpdateRequestArrray(request_array, dir_state, buffer_request_up, buffer_request_down);
        delay(delay_wait_on_floor / 10);
      }

      if (DEBUG) {
        Serial.print("Reached Floor ");
        Serial.print(target_floor);
        Serial.println(" ");
      }

      door_state = GATE_CLOSE;
      delay(100);
      time_last_floor_ = millis();
    } else {
      // Keep moving in the dir.
      delay(500);
    }
  }

  LCDPrintInfo(current_floor_, dir_state,
               door_state, target_floor);
  delay(100);
}

void LCDPrintInfo(const int current_floor,
                  const int dir_state,
                  const int door_state,
                  const int next_floor) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ON   STATE   TO");
  lcd.setCursor(0, 1);
  lcd.print(current_floor);

  if (dir_state == UP && door_state == GATE_CLOSE) {
    lcd.print("  GOING UP   ");
  } else if (dir_state == DOWN && door_state == GATE_CLOSE) {
    lcd.print("  GOING DOWN  ");
  } else if (door_state == GATE_OPEN) {
    lcd.print("  GATE OPEN  ");
  } else {
    lcd.print("             ");
  }

  if (next_floor != -1) {
    lcd.print(next_floor);
  }
}

void UpdateFloorRequestArrays(bool* buffer_request_up,
                              bool* buffer_request_down) {

  for (int i = 0; i < TOTAL_FLOORS - 1; ++i) {
    if (digitalRead(FloorRequestUpPin[i]) == 1) {
      if (!buffer_request_up[i]) {
        if (DEBUG) {
          Serial.print("New request to go UP from floors ");
          Serial.print(i);
        }

        buffer_request_up[i] = true;
        buffer_request_count_up = buffer_request_count_up + 1;
      }
    }
    if (digitalRead(FloorRequestDownPin[i]) == 1) {
      if (!buffer_request_down[i + 1]) {
        if (DEBUG) {
          Serial.print("New request to go DOWN from floors ");
          Serial.print(i + 1);
        }

        buffer_request_down[i + 1] = true;
        buffer_request_count_down = buffer_request_count_down + 1;
      }
    }
  }
}

void UpdateRequestArrray(bool* request_array,
                         int dir_state,
                         bool* buffer_request_up,
                         bool* buffer_request_down) {
  for (int i = 0; i < TOTAL_FLOORS; ++i) {
    if (digitalRead(LiftRequestPin[i]) == 1
        && request_array[i] == false) {
      request_array[i] = true;
      request_count = request_count + 1;

      if (DEBUG) {
        Serial.print("New request from lift to floor ");
        Serial.print(i);
      }
    }
  }

  if (dir_state == STOP) {
    for (int i = 0; i < TOTAL_FLOORS; ++i) {
      if (buffer_request_up[i] == true) {
        buffer_request_count_up = buffer_request_count_up - 1;
        buffer_request_up[i] == false;
        if (request_array[i] == false) {
          request_array[i] = true;
          request_count = request_count + 1;
        }
      }
      if (buffer_request_down[i] == true) {
        buffer_request_count_down = buffer_request_count_down - 1;
        buffer_request_down[i] == false;
        if (request_array[i] == false) {
          request_array[i] = true;
          request_count = request_count + 1;
        }
      }
    }
  } else if (dir_state == UP) {
    for (int i = 0; i < TOTAL_FLOORS; ++i) {
      if (buffer_request_up[i] == true) {
        buffer_request_count_up = buffer_request_count_up - 1;
        buffer_request_up[i] == false;
        if (request_array[i] == false) {
          request_array[i] = true;
          request_count = request_count + 1;
        }
      }
    }
  } else {
    for (int i = 0; i < TOTAL_FLOORS; ++i) {
      if (buffer_request_down[i] == true) {
        buffer_request_count_down = buffer_request_count_down - 1;
        buffer_request_down[i] == false;
        if (request_array[i] == false) {
          request_array[i] = true;
          request_count = request_count + 1;
        }
      }
    }
  }

  if (DEBUG) {
    Serial.print("Request Array (in dir.): ");
    for (int i = 0; i < TOTAL_FLOORS; ++i) {
      if (request_array[i]) {
        Serial.print(" ");
        Serial.print(i);
      }
    }
    Serial.println(" ");
    if (dir_state == UP) {
      Serial.print("Buffer requests in DOWN dir: ");
      if (buffer_request_count_down) {
        for (int i = 0; i < TOTAL_FLOORS; ++i) {
          if (buffer_request_down[i]) {
            Serial.print(" ");
            Serial.print(i);
          }
        }
      }
      Serial.println(" ");
    } else if (dir_state == DOWN) {
      Serial.print("Buffer requests in UP dir: ");
      if (buffer_request_count_up) {
        for (int i = 0; i < TOTAL_FLOORS; ++i) {
          if (buffer_request_up[i]) {
            Serial.print(" ");
            Serial.print(i);
          }
        }
      }
      Serial.println(" ");
    } else {
      if (buffer_request_count_down) {
        Serial.print("Buffer requests in DOWN dir: ");
        for (int i = 0; i < TOTAL_FLOORS; ++i) {
          if (buffer_request_down[i]) {
            Serial.print(" ");
            Serial.print(i);
          }
        }
        Serial.println(" ");
      }
      if (buffer_request_count_up) {
        for (int i = 0; i < TOTAL_FLOORS; ++i) {
          if (buffer_request_up[i]) {
            Serial.print(" ");
            Serial.print(i);
          }
        }
        Serial.println(" ");
      }
    }
  }
}

int GetFirstElementInRequestArrayInDirection(bool *request_array,
    const int &dir_state,
    const int &current_floor) {
  if (dir_state == UP) {
    for (int i = current_floor; i < TOTAL_FLOORS; ++i) {
      if (request_array[i])
        return i;
    }
  }
  else if (dir_state == DOWN) {
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
                              int &dir_state,
                              int &target_floor,
                              int &offset) {
  if (dir_state == STOP) {
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
        dir_state = UP;
        offset = closest_floor_up - current_floor;

        return;
      }
      else {
        target_floor = closest_floor_down;
        dir_state = DOWN;
        offset = current_floor - closest_floor_down;

        return;
      }
    }
    else if (closest_floor_up != -1) {
      target_floor = closest_floor_up;
      dir_state = UP;
      offset = closest_floor_up - current_floor;

      return;
    }
    else if (closest_floor_down != -1) {
      target_floor = closest_floor_down;
      dir_state = DOWN;
      offset = current_floor - closest_floor_down;

      return;
    }
    else {
      target_floor = -1;
      dir_state = STOP;
      offset = -1;

      return;
    }
  }
  else if (dir_state == UP) {
    int closest_floor_up =
      GetFirstElementInRequestArrayInDirection(
        request_array, UP, current_floor);

    if (closest_floor_up != -1) {
      target_floor = closest_floor_up;
      dir_state = UP;
      offset = closest_floor_up - current_floor;

      return;
    }
    else {
      target_floor = -1;
      dir_state = STOP;
      offset = -1;

      return;
    }
  }
  else if (dir_state == DOWN) {
    int closest_floor_down =
      GetFirstElementInRequestArrayInDirection(
        request_array, DOWN, current_floor);

    if (closest_floor_down != -1) {
      target_floor = closest_floor_down;
      dir_state = DOWN;
      offset = current_floor - closest_floor_down;

      return;
    }
    else {
      target_floor = -1;
      dir_state = STOP;
      offset = -1;

      return;
    }
  }

  return;
}


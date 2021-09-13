
// To Set Debug Print On (1) / Off (0)
#define DEBUG 1

// Set the total num of floors.
#define TOTAL_FLOORS 6
// Initialize with Button Pin.
const int buttonPinFloor[TOTAL_FLOORS] = {2, 3, 4, 5, 6, 7};
// Initialize with `current floor` indicating LED.
const int LEDPinFloor[TOTAL_FLOORS] = {13, 12, 11, 10, 9, 8};
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
const int floor_to_floor_time = 5000;

// Enum is not supported in Arduino, so using define.
// Stores the state of the lift.
#define STOP 0
#define UP 1
#define DOWN 2
int dir = STOP;

void setup()
{
  Serial.begin(9600);

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
  dir = STOP;
}

void loop()
{
  if (dir == STOP) {
    delay(1000);
    time_last_floor_ = millis();
  }

  UpdateRequestArrray(request_array);

  int target_floor = -1, offset = -1;
  GetNextTargetInDirection(request_array, current_floor_, dir, target_floor, offset);

  if (millis() - time_last_floor_ > floor_to_floor_time) {
    if (dir == UP) {
      current_floor_ = current_floor_ + 1;
    } else {
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
      dir = STOP;
      request_array[target_floor] = false;
      if (DEBUG) {
        Serial.print("Reached Floor ");
        Serial.print(target_floor);
        Serial.println(" ");
      }
      delay(1000);
      time_last_floor_ = millis();
    } else {
      // Keep moving in the dir.
      delay(1000);
    }
  }

  if (DEBUG) {
    Serial.print("State: "); Serial.print(dir); Serial.println(" ");
  }
}

void UpdateRequestArrray(bool* request_array) {

  if (DEBUG) Serial.print("Request Floors: ");

  for (int i = 0; i < TOTAL_FLOORS; ++i) {
    if (digitalRead(buttonPinFloor[i]) == 1) {
      Serial.println(i);
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
    const int &dir,
    const int &current_floor) {
  if (dir == UP) {
    for (int i = current_floor; i < TOTAL_FLOORS; ++i) {
      if (request_array[i])
        return i;
    }
  }
  else if (dir == DOWN) {
    for (int i = current_floor; i >= 0; --i) {
      if (request_array[i])
        return i;
    }
  }

  // If the current floor >= TOTAL_FLOOR - 1 && dir == UP
  // or current floor == 0 && dir == DOWN
  // or dir == STOP
  // return -1.
  return -1;
}

int abs_(const int x) {
  return (x > 0 ? x : -x);
}

void GetNextTargetInDirection(bool *request_array,
                              int &current_floor,
                              int &dir,
                              int &target_floor,
                              int &offset) {
  if (dir == STOP) {
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
        dir = UP;
        offset = closest_floor_up - current_floor;

        return;
      }
      else {
        target_floor = closest_floor_down;
        dir = DOWN;
        offset = current_floor - closest_floor_down;

        return;
      }
    }
    else if (closest_floor_up != -1) {
      target_floor = closest_floor_up;
      dir = UP;
      offset = closest_floor_up - current_floor;

      return;
    }
    else if (closest_floor_down != -1) {
      target_floor = closest_floor_down;
      dir = DOWN;
      offset = current_floor - closest_floor_down;

      return;
    }
    else {
      target_floor = -1;
      dir = STOP;
      offset = -1;

      return;
    }
  }
  else if (dir == UP) {
    int closest_floor_up =
      GetFirstElementInRequestArrayInDirection(
        request_array, UP, current_floor);

    if (closest_floor_up != -1) {
      target_floor = closest_floor_up;
      dir = UP;
      offset = closest_floor_up - current_floor;

      return;
    }
    else {
      target_floor = -1;
      dir = STOP;
      offset = -1;

      return;
    }
  }
  else if (dir == DOWN) {
    int closest_floor_down =
      GetFirstElementInRequestArrayInDirection(
        request_array, DOWN, current_floor);

    if (closest_floor_down != -1) {
      target_floor = closest_floor_down;
      dir = DOWN;
      offset = current_floor - closest_floor_down;

      return;
    }
    else {
      target_floor = -1;
      dir = STOP;
      offset = -1;

      return;
    }
  }

  return;
}


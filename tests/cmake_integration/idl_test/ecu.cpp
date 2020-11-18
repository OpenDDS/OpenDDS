#include <iostream>

#include "ecuC.h"

using namespace std;

const string gears[] = {"Park", "Reverse", "Neutral",
                        "First", "Second", "Third",
                        "Fourth", "Fifth", "Sixth"};

void display(const EcuInfo& ecu) {
  cout << "ID:           " << ecu.id << endl;
  cout << "RPM:          " << ecu.rpm.value << endl;
  cout << "Coolant:      " << ecu.coolant.value << endl;
  cout << "Oil:          " << ecu.oil.value << endl;
  cout << "Transmission: " << gears[ecu.transmission.gear] << endl;
  cout << "MPG:          " << ecu.calc.mpg << endl;
  cout << "Range:        " << ecu.calc.range << endl;
}

int main() {
  EcuInfo ecu;

  ecu.id = "FORD";
  ecu.coolant.type = COOLANT;
  ecu.coolant.value = 200;
  ecu.coolant.type = OIL;
  ecu.oil.value = 180;
  ecu.rpm.value = 2250;
  ecu.transmission.fault = OK;
  ecu.transmission.gear = THIRD;

  ecu.calc.range = 125;
  ecu.calc.mpg = 26;

  display(ecu);

  return 0;
}

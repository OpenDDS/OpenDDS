#include "carC.h"

#include <iostream>

using namespace std;

const string gears[] = { "Park", "Reverse", "Neutral",
    					 "First", "Second", "Third",
						 "Fourth", "Fifth", "Sixth" };

void display(const CarInfo& car)
{
	cout << "ID:           " << car.id << endl;
	cout << "RPM:          " << car.rpm.value << endl;
	cout << "Coolant:      " << car.coolant.value << endl;
	cout << "Oil:          " << car.oil.value << endl;
	cout << "Transmission: " << gears[car.transmission.gear] << endl;
}

int main()
{
	CarInfo car;

	car.id = "FORD";
	car.coolant.type = COOLANT;
	car.coolant.value = 200;
	car.coolant.type = OIL;
	car.oil.value = 180;
	car.rpm.value = 2250;
	car.transmission.fault = OK;
	car.transmission.gear = THIRD;

	display(car);

	return 0;
}

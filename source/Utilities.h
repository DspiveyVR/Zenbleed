/*
 * This file for utility functions
 * to be used in this project.
 */


#include <cmath>
using namespace std;

bool compareFloat(float a, float b) {
	if (abs(a-b) < 1e-9) {
		return true;
	} else {
		return false;
	}
}

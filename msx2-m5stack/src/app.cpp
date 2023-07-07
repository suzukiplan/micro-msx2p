#include "msx2.hpp"
#include <M5Core2.h>

static MSX2 msx2(MSX2_COLOR_MODE_RGB555);

void setup() {
	M5.begin();
    Serial.begin(115200);
}

void loop() {
}

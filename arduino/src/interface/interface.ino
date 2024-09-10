#include <string.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    // wait for serial message that exactly equals "ready"
    delay(2000);
    digitalWrite(LED_BUILTIN, HIGH);
    while (Serial.available() < 5) {
        delay(500);
        Serial.println("waiting");

    }
    digitalWrite(LED_BUILTIN, LOW);
    char buf[256];
    int read = Serial.readBytes(buf, 255);
    buf[read] = '\0';
    if (strncmp(buf, "ready", 5) != 0) {
        Serial.println("bad ready");
        Serial.print("Got: ");
        Serial.println(buf);
        Serial.println("Reset device...");
        delay(1000);
        while (true) {
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
    Serial.println("ready");
}

void reset() {
    void (*resetFunc)(void) = 0;
    resetFunc();
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("msg(o101010)");
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    if (Serial.available() > 0) {
        char buf[256];
        int read = Serial.readBytes(buf, 255);
        buf[read] = '\0';
        if (strncmp(buf, "shutdown", 8) == 0) {
            reset(); // TODO: this causes issues, and we cant reconnect without physically pressing the reset button even though this should reset it properly
        }
    }

            

}

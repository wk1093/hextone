#include <string.h>

#define SWITCH_1 12
bool sw1_state = false;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SWITCH_1, INPUT_PULLUP);
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

long long lastPing = 0;

void loop() {
    bool sws;
    if ((sws = !digitalRead(SWITCH_1)) != sw1_state) {
        sw1_state = sws;
        if (sws) {
            Serial.print("msg(o1)");
        } else {
            Serial.print("msg(f1)");
        }
    }

    if (millis() - lastPing > 1000) {
        Serial.print("msg(ping)");
        lastPing = millis();
    }


    // if (Serial.available() > 0) {
    //     char buf[256];
    //     int read = Serial.readBytes(buf, 255);
    //     buf[read] = '\0';
    //     if (strncmp(buf, "shutdown", 8) == 0) {
    //         // TODO: reset without fucking up the serial communitcation
    //     }
    // }

            

}

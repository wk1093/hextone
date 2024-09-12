#include <string.h>

const int SWITCH_1 = 11;
const int SWITCH_2 = 10;
const int SWITCH_3 = 9;
const int SWITCH_4 = 8;
const int SWITCH_5 = 7;
const int SWITCH_6 = 6;
const int SWITCH_7 = 5;
const int SWITCH_8 = 4;
bool sw1_state = false;
bool sw2_state = false;
bool sw3_state = false;
bool sw4_state = false;
bool sw5_state = false;
bool sw6_state = false;
bool sw7_state = false;
bool sw8_state = false;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SWITCH_1, INPUT_PULLUP);
    pinMode(SWITCH_2, INPUT_PULLUP);
    pinMode(SWITCH_3, INPUT_PULLUP);
    pinMode(SWITCH_4, INPUT_PULLUP);
    pinMode(SWITCH_5, INPUT_PULLUP);
    pinMode(SWITCH_6, INPUT_PULLUP);
    pinMode(SWITCH_7, INPUT_PULLUP);
    pinMode(SWITCH_8, INPUT_PULLUP);
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
long long lastsw1 = 0;
long long lastsw2 = 0;
long long lastsw3 = 0;
long long lastsw4 = 0;
long long lastsw5 = 0;
long long lastsw6 = 0;
long long lastsw7 = 0;
long long lastsw8 = 0;


void loop() {
#define SWTICHCHECK(num) \
    bool sw##num##s = digitalRead(SWITCH_##num); \
    bool sw##num##db; \
    if ((sw##num##s != sw##num##_state) && (millis() - lastsw##num > 25)) { \
        sw##num##db = sw##num##s; \
        lastsw##num = millis(); \
    } else { \
        sw##num##db = sw##num##_state; \
    } \
    if ((sws = !sw##num##db) != !sw##num##_state) { \
        sw##num##_state = !sws; \
        if (sws) { \
            Serial.print("msg(o" #num ")"); \
        } else { \
            Serial.print("msg(f" #num ")"); \
        } \
        lastPing = millis(); \
    }

    bool sws;

    SWTICHCHECK(1);
    SWTICHCHECK(2);
    SWTICHCHECK(3);
    SWTICHCHECK(4);
    SWTICHCHECK(5);
    SWTICHCHECK(6);
    SWTICHCHECK(7);
    SWTICHCHECK(8);




    if (millis() - lastPing > 1000) {
        Serial.print("msg(ping)");
        lastPing = millis();
    }


    if (Serial.available() > 0) {
        char buf[256];
        int read = Serial.readBytes(buf, 255);
        buf[read] = '\0';
        if (strncmp(buf, "shutdown", 8) == 0) {
            // TODO: reset without fucking up the serial communitcation
            setup();
        }
    }

            

}

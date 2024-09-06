
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
    char buf[6];
    Serial.readBytes(buf, 5);
    buf[5] = '\0';
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

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("ping");
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}

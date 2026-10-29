#include "SerialConsole.h"
void SerialConsole::update(void (*handler)(const char*)) {
    // Bound work per loop so a stream of serial data cannot starve animation.
    for (int budget=0; budget<64 && Serial.available(); ++budget) {
        char c=Serial.read();
        if (c=='\r' || c=='\n') {
            if (overflow) Serial.println("ERR command too long; discarded");
            else if (used) { buffer[used]=0; handler(buffer); }
            used=0; overflow=false;
        } else if (c==8 || c==127) {
            if (used && !overflow) --used;
        } else if (c>=32 && c<=126 && !overflow) {
            if (used+1<sizeof(buffer)) buffer[used++]=c;
            else overflow=true;
        }
    }
}

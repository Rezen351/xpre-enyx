#include "Application.h"
#include "Config.h"
#include "DeviceBus.h"
#include "Hardware.h"
#include "XpreOLED.h"
#include "SerialConsole.h"
#include "Diagnostics.h"
#include "BleLink.h"
#include <Preferences.h>
#include <esp_sleep.h>

namespace {
Hardware hw;
XpreOLED oled;
SerialConsole console;
bool oledReady=false;
Diagnostics diagnostics(hw,oled,oledReady);
Preferences settings;
bool settingsReady=false, stream=false, menu=false, selectingMood=false, dimmed=false;
uint8_t selection=0;
Mood selectedMood=Mood::Auto;
float sensitivity=2.0f;
uint32_t lastProbe=0,lastReport=0,lastUi=0,lastActivity=0,notificationAt=0;
bool notificationActive=false;
char notification[97]="No notifications";
const char* moodNames[]={"auto","normal","curious","happy","surprised","love","sleepy","wink"};
const char* menuNames[]={"Face","Emotion","Sensors","Notifications","Sound"};

void save() {
    if (!settingsReady) return;
    settings.putUChar("mood",static_cast<uint8_t>(selectedMood));
    settings.putBool("sound",hw.sound);
    settings.putFloat("sensitivity",sensitivity);
}
void activity() { lastActivity=millis(); if (dimmed && oledReady) oled.dim(false); dimmed=false; }
void probeOLED() {
    Serial.println("OLED: probing address");
    if (!i2cDetected(oledAddress)) {
        if (i2cDetected(0x3C)) oledAddress=0x3C;
        else if (i2cDetected(0x3D)) oledAddress=0x3D;
    }
    Serial.printf("OLED: probe result 0x%02X\n", oledAddress);
    oledReady=oled.begin();
}
void status() {
    Serial.printf("Xpre-Enyx uptime=%lu s heap=%u bytes | OLED 0x%02X: %s\n",static_cast<unsigned long>(millis()/1000),ESP.getFreeHeap(),oledAddress,oledReady?"ACK/init OK; pixels unverified":"UNAVAILABLE");
    Serial.printf("I2C SDA=%u SCL=%u levels=%d/%d | BLE=%s | mood=%s sensitivity=%.1f\n",Config::sda,Config::scl,digitalRead(Config::sda),digitalRead(Config::scl),BleLink::connected()?"CONNECTED":"ADVERTISING",moodNames[static_cast<uint8_t>(selectedMood)],sensitivity);
    hw.report();
}
void help() {
    Serial.println("Xpre-Enyx commands (115200 baud; Enter/LF/CRLF):");
    Serial.println("help | status | stream on/off | test [all|oled|mpu|dht|audio|encoder|i2c|stop]");
    Serial.println("mood auto|normal|curious|happy|surprised|love|sleepy|wink");
    Serial.println("notify <text> | sound on/off | beep | calibrate | sensitivity 0.5..8");
    Serial.println("retry | menu | face | sleep | reboot");
    Serial.println("test: interactive diagnostic ~14 seconds; test stop cancels. Missing hardware never stops the app.");
}
void notify(const char* text) {
    strlcpy(notification,text,sizeof(notification));
    for (char* c=notification;*c;++c) if (*c<32 || *c>126) *c=' ';
    notificationAt=millis(); notificationActive=true;
    activity(); hw.beep(1600,110);
    Serial.printf("NOTIFICATION: %s\n",notification);
    BleLink::publish("notification received");
}
void command(const char* input) {
    char line[Config::commandSize]; strlcpy(line,input,sizeof(line));
    char* start=line; while (*start==' ') ++start;
    char* end=start+strlen(start); while(end>start && end[-1]==' ') *--end=0;
    if (!*start) return;
    char* arg=strchr(start,' ');
    if (arg) { *arg++=0; while(*arg==' ')++arg; }
    else arg=end;
    activity();
    if (!strcmp(start,"help")) help();
    else if (!strcmp(start,"status")) status();
    else if (!strcmp(start,"test")) diagnostics.start(*arg?arg:"all");
    else if (!strcmp(start,"stream") && (!strcmp(arg,"on")||!strcmp(arg,"off"))) {
        stream=!strcmp(arg,"on"); Serial.printf("Stream %s\n",stream?"ON":"OFF");
    } else if (!strcmp(start,"mood")) {
        bool found=false;
        for (uint8_t i=0;i<8;++i) if(!strcmp(arg,moodNames[i])) { selectedMood=static_cast<Mood>(i);found=true;break; }
        if (found) { save(); Serial.printf("Mood %s\n",arg); }
        else Serial.println("ERR invalid mood; type help");
    } else if (!strcmp(start,"sound") && (!strcmp(arg,"on")||!strcmp(arg,"off"))) {
        hw.sound=!strcmp(arg,"on"); if (!hw.sound) ledcWriteTone(0,0); save();
        Serial.printf("Sound %s\n",hw.sound?"ON":"OFF");
    } else if (!strcmp(start,"beep")) { hw.beep(); Serial.println("Beep requested; verify by listening (respects mute)."); }
    else if (!strcmp(start,"notify") && *arg) notify(arg);
    else if (!strcmp(start,"calibrate")) hw.calibrate();
    else if (!strcmp(start,"sensitivity")) {
        char* tail=nullptr; float value=strtof(arg,&tail);
        if (tail!=arg && !*tail && isfinite(value) && value>=0.5f && value<=8.0f) {
            sensitivity=value;save();Serial.printf("Sensitivity %.1f m/s2\n",value);
        } else Serial.println("ERR sensitivity must be 0.5..8");
    } else if (!strcmp(start,"retry")) { hw.probe();probeOLED();status(); }
    else if (!strcmp(start,"menu")) { menu=true; selectingMood=false; selection=0; Serial.println("Menu open"); }
    else if (!strcmp(start,"face")) { menu=false; selectingMood=false; selection=0; Serial.println("Face mode"); }
    else if (!strcmp(start,"sleep")) {
        if (!digitalRead(Config::button)) { Serial.println("Release encoder button before sleep.");return; }
        esp_err_t result=esp_deep_sleep_enable_gpio_wakeup(1ULL<<Config::button,ESP_GPIO_WAKEUP_GPIO_LOW);
        if (result!=ESP_OK) { Serial.printf("Sleep wake setup failed: %d\n",result);return; }
        Serial.println("Deep sleep: USB/BLE disconnect. Wake with GPIO4 button or RESET.");
        Serial.flush();
        if(oledReady) oled.testPattern(1);
        ledcWriteTone(0,0);
        esp_deep_sleep_start();
    } else if (!strcmp(start,"reboot")) { Serial.println("Rebooting"); Serial.flush();ESP.restart(); }
    else Serial.println("ERR unknown command/arguments; type help");
}
void drawUi(uint32_t now) {
    if (!oledReady || diagnostics.active()) return;
    if (notificationActive && now-notificationAt<5000) {
        if (now-lastUi>=150) { lastUi=now;oled.text("MESSAGE",notification); }
        return;
    }
    notificationActive=false;
    if (menu) {
        if(now-lastUi<150)return;lastUi=now;
        char body[150];
        if(selectingMood) {
            snprintf(body,sizeof(body),"< %s >\nClick: save\nHold: back",moodNames[selection]);
            oled.text("EMOTION",body);
        } else {
            switch(selection) {
                case 0: snprintf(body,sizeof(body),"> Face\nClick: show character\nRotate: next page");break;
                case 1: snprintf(body,sizeof(body),"> Emotion: %s\nClick: choose mode",moodNames[static_cast<uint8_t>(selectedMood)]);break;
                case 2:
                    if(hw.dhtReady) snprintf(body,sizeof(body),"%.1f C / %.1f %%RH\nMPU: %s\nMotion: %.2f m/s2\nClick: calibrate",hw.temperature,hw.humidity,hw.mpuReady?"OK":"missing",hw.motion);
                    else snprintf(body,sizeof(body),"DHT11: no data\nMPU: %s\nMotion: %.2f m/s2\nClick: calibrate",hw.mpuReady?"OK":"missing",hw.motion);
                    break;
                case 3: snprintf(body,sizeof(body),"%s",notification);break;
                default: snprintf(body,sizeof(body),"Sound: %s\nClick: toggle\nBLE: %s",hw.sound?"ON":"OFF",BleLink::connected()?"connected":"advertising");break;
            }
            oled.text(menuNames[selection],body);
        }
    } else {
        Mood mood=selectedMood;
        if(mood==Mood::Auto) {
            if(now-lastActivity>=Config::idleSleep) mood=Mood::Sleepy;
            else if(hw.mpuReady && hw.motion>sensitivity*2) mood=Mood::Surprised;
            else if(hw.mpuReady && hw.motion>sensitivity) mood=Mood::Happy;
        }
        oled.setMood(mood);oled.animate(now);
    }
}
}
void appSetup() {
    Serial.begin(115200);
    Serial.println("BOOT: serial ready");
    Wire.begin(Config::sda,Config::scl,Config::i2cClock);
    Wire.setTimeOut(Config::i2cTimeout);
    Serial.println("BOOT: I2C ready");
    settingsReady=settings.begin("xpre-enyx",false);
    uint8_t stored=settingsReady?settings.getUChar("mood",0):0;
    selectedMood=static_cast<Mood>(stored<8?stored:0);
    hw.sound=settingsReady?settings.getBool("sound",true):true;
    sensitivity=settingsReady?settings.getFloat("sensitivity",2):2;
    if(!isfinite(sensitivity)||sensitivity<0.5||sensitivity>8)sensitivity=2;
    Serial.println("BOOT: hardware init");
    hw.begin();
    Serial.println("BOOT: hardware done");
    probeOLED();
    Serial.printf("BOOT: OLED init %s\n", oledReady ? "done" : "unavailable");
    BleLink::begin();
    lastActivity=millis();lastProbe=millis();
    help();status();
}
void appLoop() {
    console.update(command);
    BleLink::update();
    char message[97];if(BleLink::takeMessage(message,sizeof(message))) notify(message);
    uint32_t now=millis();
    hw.update(now);
    int steps=hw.takeSteps();bool click=hw.takeClick(),hold=hw.takeHold();
    if(steps || click || hold) {
        activity();
        if(!diagnostics.active()) {
            if(hold) { menu=!menu; selectingMood=false;selection=0; }
            else if(menu) {
                int count=selectingMood?8:5;
                selection=((int(selection)+steps)%count+count)%count;
                if(click) {
                    hw.beep();
                    if(selectingMood) { selectedMood=static_cast<Mood>(selection);save();selectingMood=false;selection=1; }
                    else if(selection==0)menu=false;
                    else if(selection==1){selectingMood=true;selection=static_cast<uint8_t>(selectedMood);}
                    else if(selection==2)hw.calibrate();
                    else if(selection==4){hw.sound=!hw.sound;save();}
                }
            } else if(click) {menu=true;selection=0;}
        }
        Serial.printf("INPUT position=%ld clicks=%lu\n",long(hw.encoderPosition),static_cast<unsigned long>(hw.clicks));
    }
    now=millis();
    diagnostics.update(now);
    if(!diagnostics.active() && now-lastProbe>=Config::retryInterval) {
        lastProbe=now;
        if(!oledReady) probeOLED();
        if(!hw.mpuReady)hw.probe();
    }
    if(!dimmed && now-lastActivity>=Config::idleSleep) {dimmed=true;if(oledReady)oled.dim(true);}
    if(stream && now-lastReport>=2000){lastReport=now;status();}
    drawUi(now);
    delay(1);
}

#include <cassert>
#include <iostream>
#include "Arduino.h"
#include "Wire.h"
uint32_t clockMs=0;
SerialMock Serial;
TwoWire Wire;
#include "../attenuator.ino"
void cmd(const char *s) { char b[100]; strcpy(b,s); execute(b); }
void tick(uint32_t dt=1000) { clockMs+=dt; animate(); }
int main() {
 setup(); assert(ready && mode==SELF_TEST);
 assert(Wire.timeoutUs==25000 && Wire.resetOnTimeout);
 tick(); assert(logicalImage==0xfffffffUL);
 tick(); assert(logicalImage==0);
 tick(); assert(mode==VOLUME && logicalImage==0x3fff);
 cmd("off"); assert(mode==IDLE && logicalImage==0);
 cmd("fill 28"); assert(logicalImage==0xfffffffUL);
 for (int i=0;i<8;i++) assert(Wire.ram[1+2*i]==(i<4?127:0));
 cmd("fill 29"); assert(logicalImage==0xfffffffUL);
 cmd("fill -1"); assert(logicalImage==0xfffffffUL);
 cmd("fill 1 junk"); assert(logicalImage==0xfffffffUL);
 cmd("fill 1"); assert(Wire.ram[1]==1);
 cmd("invert"); assert(Wire.ram[7]==64 && Wire.ram[1]==0);
 cmd("invert"); Serial.incoming="discover\r\n"; readSerial();
 assert(frame==1);
 for(int i=0;i<28;i++) {
   tick(60000); assert(mode==DISCOVER && frame==i+1); int n=0;
   for(int j=1;j<=16;j++) n+=__builtin_popcount(Wire.ram[j]);
   assert(n==1 && Wire.ram[1+2*(i/7)]==(1<<(i%7)));
   Serial.incoming="\r\n"; readSerial();
 }
 tick(); assert(mode==IDLE && logicalImage==0);
 cmd(""); assert(mode==IDLE && logicalImage==0);
 cmd("discover"); assert(frame==1);
 cmd("next junk"); assert(frame==1);
 Serial.incoming="\r\n"; readSerial(); assert(frame==2);
 Serial.incoming="\r"; readSerial(); assert(frame==3);
 Serial.incoming="\n"; readSerial(); assert(frame==3);
 Serial.incoming="\n"; readSerial(); assert(frame==4);
 Serial.incoming="\r\r"; readSerial(); assert(frame==6);
 Serial.incoming=std::string(60,'x')+"\r\n";
 while(Serial.available()) readSerial();
 assert(frame==6);
 cmd("discover"); assert(frame==1);
 cmd("off"); tick(60000); assert(mode==IDLE && logicalImage==0);
 cmd("test"); for(int i=0;i<28;i++) { tick(); assert(logicalImage==(1UL<<i)); }
 tick(); assert(mode==IDLE && logicalImage==0);
 cmd("bounce"); for(int i=0;i<55;i++) { tick(); int expected=i<=27?i:54-i; assert(logicalImage==(1UL<<expected)); }
 cmd("chase"); for(int i=0;i<57;i++) { tick(); assert(logicalImage==(1UL<<(i%28))); }
 cmd("demo"); for(int i=0;i<57;i++) { tick(); int n=i<=28?i:56-i; assert(logicalImage==((1UL<<n)-1)); }
 cmd("brightness 15"); assert(brightness==15);
 cmd("brightness 16"); assert(brightness==15);
 cmd("speed 20"); assert(stepMs==20);
 cmd("speed 0"); assert(stepMs==20);
 cmd("chase"); tick(); clockMs=0xfffffff0UL; lastStep=clockMs; clockMs=10; animate(); assert(frame==2);
 Serial.incoming=std::string(60,'x')+"off\n"; while(Serial.available()) readSerial(); assert(mode==CHASE);
 Serial.incoming="off\r\n"; readSerial(); assert(mode==IDLE);
 Wire.connected=false; cmd("all"); assert(!ready && mode==IDLE);
 cmd("scan"); assert(!ready);
 Wire.connected=true; cmd("scan"); assert(ready && logicalImage==((1UL<<volumeDisplay.count())-1));
 // Volume scaling exercises all 101 values and all 28 physical positions.
 for (int v=0;v<=100;v++) {
   char line[20]; snprintf(line,sizeof(line),"volume %d",v); cmd(line);
   int count=(v*28+50)/100;
   assert(volumeDisplay.volume()==v && mode==VOLUME && logicalImage==((1UL<<count)-1));
   for(int row=0;row<4;row++) {
     int expected=0;
     for(int bit=0;bit<7;bit++) if(bit*4+row<count) expected|=1<<bit;
     assert(Wire.ram[1+2*row]==expected);
   }
 }
 cmd("volume 101"); cmd("volume -1"); cmd("volume"); cmd("volume 5 junk");
 assert(volumeDisplay.volume()==100);
 cmd("volume 50"); cmd("mute");
 assert(volumeDisplay.muted() && volumeDisplay.volume()==50);
 tick(250); assert(logicalImage==(3UL<<12));
 for(int cap=0;cap<=15;cap++) {
   char line[20]; snprintf(line,sizeof(line),"brightness %d",cap); cmd(line);
   int lo=15,hi=0;
   for(int t=0;t<=2500;t+=10) {
     tick(10); assert(appliedBrightness<=cap);
     assert(Wire.enabled==(cap!=0));
     lo=std::min(lo,int(appliedBrightness)); hi=std::max(hi,int(appliedBrightness));
     assert(logicalImage==(3UL<<12));
   }
   assert(lo==(cap<=3?cap:3) && hi==cap);
 }
 cmd("volume 0"); assert(logicalImage==3 && volumeDisplay.muted());
 cmd("volume 100"); assert(logicalImage==(3UL<<26));
 cmd("unmute"); tick(249); assert(logicalImage!=0xfffffffUL);
 tick(1); assert(logicalImage==0xfffffffUL && appliedBrightness==15);
 cmd("volume 0"); cmd("mute"); tick(250); assert(logicalImage==3);
 cmd("unmute"); tick(250); assert(logicalImage==0);
 cmd("volume 75"); cmd("mute"); tick(250);
 cmd("purge"); cmd("volume 50"); assert(mode==VENT);
 tick(VentAnimation::DURATION_MS); assert(mode==VOLUME && volumeDisplay.muted() && logicalImage==(3UL<<12));
 cmd("vent"); cmd("unmute"); cmd("volume 25"); tick(VentAnimation::DURATION_MS);
 assert(mode==VOLUME && !volumeDisplay.muted() && logicalImage==127);
 cmd("vent"); cmd("off"); tick(2000); assert(mode==IDLE && logicalImage==0);
 cmd("show"); assert(logicalImage==127);
 clockMs=0xffffff80UL; cmd("mute"); tick(250); assert(logicalImage==(3UL<<5));
 tick(1250); assert(appliedBrightness==15);
 tick(1250); assert(appliedBrightness==3);
 cmd("unmute"); tick(250); assert(logicalImage==127 && appliedBrightness==15);
 cmd("mute"); tick(250); cmd("chase"); assert(appliedBrightness==15);
 cmd("show"); cmd("brightness 0"); assert(!Wire.enabled);
 cmd("brightness 12"); assert(Wire.enabled && appliedBrightness<=12);
 cmd("vent"); Wire.connected=false; tick(40); assert(!ready && mode==IDLE);
 Wire.connected=true; cmd("scan"); assert(ready && mode==VOLUME && volumeDisplay.volume()==25);
 // Mute never adds a segment above a bar with at least two lit segments.
 for (int v=0;v<=100;v++) {
   VolumeDisplay state;
   state.setVolume(v);
   uint32_t bar=state.render(0,12).image;
   state.setMuted(true,0);
   for (int t=0;t<=250;t++) {
     uint32_t image=state.render(t,12).image;
     if(state.count()>=2) assert((image & ~bar)==0);
   }
   uint8_t start=state.count()>=2?state.count()-2:0;
   assert(state.render(250,12).image==(3UL<<start));
 }
 // The marker pair stays fixed while the lower bar drains down/refills up.
 VolumeDisplay directionCheck;
 directionCheck.setVolume(50);
 directionCheck.setMuted(true,0);
 assert(directionCheck.render(0,12).image==0x3fff);
 assert(directionCheck.render(125,12).image==((3UL<<12)|0x3f));
 assert(directionCheck.render(250,12).image==(3UL<<12));
 directionCheck.setMuted(false,250);
 assert(directionCheck.render(250,12).image==(3UL<<12));
 assert(directionCheck.render(375,12).image==((3UL<<12)|0x3f));
 assert(directionCheck.render(500,12).image==0x3fff);
 // ISR quadrature: 11 -> 01 -> 00 -> 10 -> 11 is positive.
 auto phases=[](int p) { inputPins[2]=(p>>1)&1; inputPins[3]=p&1; encoderEdge(); };
 auto forward=[&]() { phases(1); phases(0); phases(2); phases(3); readEncoder(); };
 auto backward=[&]() { phases(2); phases(0); phases(1); phases(3); readEncoder(); };
 cmd("unmute"); cmd("volume 50"); forward(); assert(volumeDisplay.volume()==54);
 backward(); assert(volumeDisplay.volume()==50);
 phases(1); phases(3); phases(1); phases(0); phases(2); phases(3); readEncoder();
 assert(volumeDisplay.volume()==54); // Bounce produced only one step.
 phases(0); phases(3); readEncoder(); assert(volumeDisplay.volume()==54);
 cmd("volume 100"); forward(); assert(volumeDisplay.volume()==100);
 backward(); assert(volumeDisplay.volume()==96);
 cmd("volume 0"); backward(); assert(volumeDisplay.volume()==0);
 // Button bounce, held press, release, second press, and timer wraparound.
 inputPins[4]=0; readEncoder(); clockMs+=10; inputPins[4]=1; readEncoder();
 inputPins[4]=0; readEncoder(); clockMs+=24; readEncoder(); assert(!volumeDisplay.muted());
 clockMs+=1; readEncoder(); assert(volumeDisplay.muted());
 clockMs+=1000; readEncoder(); assert(volumeDisplay.muted());
 forward(); assert(volumeDisplay.volume()==4 && volumeDisplay.muted());
 inputPins[4]=1; readEncoder(); clockMs+=25; readEncoder();
 clockMs=0xfffffff0UL; inputPins[4]=0; readEncoder(); clockMs+=25; readEncoder();
 assert(!volumeDisplay.muted());
 cmd("vent"); forward(); assert(mode==VENT && volumeDisplay.volume()==8);
 tick(VentAnimation::DURATION_MS); assert(mode==VOLUME);
 // Long idle and millis wrap must not suppress subsequent input.
 cmd("unmute"); cmd("volume 50");
 clockMs += 3600000UL; readEncoder(); forward(); assert(volumeDisplay.volume()==54);
 clockMs=0xfffffff0UL; readEncoder(); clockMs+=100; backward();
 assert(volumeDisplay.volume()==50);
 Serial.txSpace=0; forward(); assert(volumeDisplay.volume()==54); Serial.txSpace=64;
 // A transient I2C failure recovers automatically with the latest saved state.
 Wire.connected=false; cmd("all"); assert(!ready);
 forward(); assert(volumeDisplay.volume()==58);
 lastDisplayRetry=clockMs; clockMs+=1000; recoverDisplay(); assert(!ready);
 Wire.connected=true; clockMs+=999; recoverDisplay(); assert(!ready);
 clockMs+=1; recoverDisplay(); assert(ready && mode==VOLUME);
 assert(logicalImage==VolumeDisplay::fill(volumeDisplay.count()));
 EncoderInput startup;
 startup.begin(3,true,0); assert(!startup.button(true,100));
 startup.button(false,101); startup.button(false,126);
 startup.button(true,127); assert(startup.button(true,152));
 // Heartbeat is independent of the display and survives millis wraparound.
 lastHeartbeat=0xffffff00UL; clockMs=lastHeartbeat; heartbeatOn=false;
 clockMs+=500; heartbeat(); assert(heartbeatPin==HIGH);
 clockMs+=500; heartbeat(); assert(heartbeatPin==LOW);
 ready=false; Serial.output.clear(); cmd("status");
 assert(Serial.output.find("ready=0")!=std::string::npos);
 assert(Serial.output.find("AB=")!=std::string::npos);
 Serial.txSpace=0; Serial.output.clear(); cmd("status"); assert(Serial.output.empty());
 Serial.txSpace=64;
 // Independent timer detects a stalled phase and retains the last fault.
 for (int phase=1;phase<=6;phase++) {
   LoopDiagnostics diag;
   diag.phase=phase;
   for(int t=0;t<1000;t++) { diag.progressed=true; diag.tick(); }
   assert(diag.stalledPhase==0);
   for(int t=0;t<199;t++) diag.tick();
   assert(diag.stalledPhase==0);
   diag.tick(); assert(diag.stalledPhase==phase && diag.stalls==1);
   int rising=0; bool on=false;
   for(int t=0;t<200;t++) {
     bool next=diag.faultLedOn(); if(next&&!on) ++rising;
     on=next; diag.tick();
   }
   assert(rising==phase);
   diag.progressed=true; diag.tick();
   assert(diag.stalledPhase==0 && diag.lastStall==phase);
 }
 Serial.output.clear(); cmd("diag"); assert(Serial.output.find("twiTimeout=0")!=std::string::npos);
 ready=true; cmd("unmute"); tick(250); cmd("volume 50");
 auto switchPin=[](int pin,int value) { inputPins[pin]=value; readToggles(); clockMs+=25; readToggles(); };
 switchPin(5,0); assert(themePlaying);
 tick(120); assert(logicalImage==VolumeDisplay::fill(themeHeight(clockMs-themeStarted)));
 cmd("mute"); tick(250); assert(logicalImage==(3UL<<12) && themePlaying);
 switchPin(6,0); assert(mode==VENT);
 switchPin(6,1); assert(mode==VENT); // OFF does not abort.
 tick(VentAnimation::DURATION_MS); assert(mode==VOLUME && logicalImage==(3UL<<12));
 cmd("unmute"); tick(250); assert(themePlaying);
 switchPin(5,1); assert(!themePlaying && logicalImage==0x3fff);
 switchPin(6,0); tick(VentAnimation::DURATION_MS); assert(mode==VOLUME);
 readToggles(); assert(mode==VOLUME); // Holding ON cannot repeat vent.
 switchPin(5,0); clockMs=themeStarted+THEME_DURATION_MS; readToggles();
 assert(!themePlaying); readToggles(); assert(!themePlaying);
 ToggleInput bootToggle; bootToggle.begin(true,0);
 assert(bootToggle.update(true,1000)==-1);
 assert(bootToggle.update(false,1001)==-1);
 assert(bootToggle.update(false,1026)==0);
 assert(bootToggle.update(true,1027)==-1);
 assert(bootToggle.update(false,1030)==-1);
 assert(bootToggle.update(true,1031)==-1);
 assert(bootToggle.update(true,1056)==1);
 ToggleInput wrapToggle; wrapToggle.begin(false,0xfffffff0UL);
 wrapToggle.update(true,0xfffffff0UL); assert(wrapToggle.update(true,9)==1);
 for(uint32_t t=0;t<4000;t++) assert(themeHeight(t)<=28);
 // V2 phase boundaries, deterministic timing, preserved initial frames.
 for(uint32_t initial : {0UL,0x3fffUL,0xfffffffUL,3UL<<12}) {
   VentAnimation v; v.begin(0,initial);
   assert(v.render(0).image==initial);
   assert(v.render(144).image==0xfffffffUL);
   assert(v.render(179).phase==VentAnimation::VENT_BUILDUP);
   assert(v.render(180).phase==VentAnimation::VENT_CHATTER);
   assert(v.render(429).phase==VentAnimation::VENT_CHATTER);
   assert(v.render(430).phase==VentAnimation::VENT_DUMP);
   assert(v.render(1129).image==0);
   assert(v.render(1130).phase==VentAnimation::VENT_RESIDUAL);
   for(int t=1430;t<1510;t++) assert(v.render(t).image==0);
   assert(v.render(1510).phase==VentAnimation::VENT_COMPLETE);
 }
 VentAnimation v; v.begin(0,0);
 assert(__builtin_popcount(v.render(599).image)==23);
 assert(__builtin_popcount(v.render(600).image)==25); // First kickback.
 assert(__builtin_popcount(v.render(814).image)==14);
 assert(__builtin_popcount(v.render(815).image)==15); // Second kickback.
 v.begin(0xffffff00UL,127); assert(v.render(uint32_t(0xffffff00UL+1510)).phase==VentAnimation::VENT_COMPLETE);
 ready=true; cmd("unmute"); tick(250); cmd("stoptheme"); cmd("volume 50");
 cmd("vent"); assert(logicalImage==0x3fff); tick(200);
 uint32_t restartImage=logicalImage; cmd("purge"); assert(logicalImage==restartImage);
 tick(1510); assert(mode==VOLUME && volumeDisplay.volume()==50);
 cmd("vent"); tick(1509); cmd("volume 75"); cmd("mute"); tick(1);
 assert(mode==VOLUME && volumeDisplay.muted() && logicalImage==(3UL<<19));
 cmd("vent"); tick(1509); cmd("unmute"); cmd("volume 25"); tick(1);
 assert(mode==VOLUME && !volumeDisplay.muted() && logicalImage==127);
 for(int cap=0;cap<=15;cap++) {
   char line[24]; snprintf(line,sizeof(line),"brightness %d",cap); cmd(line);
   cmd("vent");
   for(int t=0;t<1510;t+=10) { tick(10); assert(appliedBrightness<=cap); assert(Wire.enabled==(cap!=0)); }
 }
 cmd("brightness 12"); cmd("invert"); cmd("vent"); tick(36);
 for(int i=0;i<28;i++) {
   const SegmentPosition &p=SEGMENT_MAP[27-i];
   assert(display.isSet(p.row,p.bit)==bool(logicalImage&(1UL<<i)));
 }
 cmd("off"); tick(2000); assert(mode==IDLE && logicalImage==0);
 std::cout << "PASS: V2 vent phases/boundaries/restart/brightness/restoration, toggle debounce/startup/vent/theme, encoder quadrature/button/clamping, V1 mute boundaries, transitions, brightness caps, override restoration, wraparound, and existing diagnostics\n";
}

#include <cassert>
#include <iostream>
#include "Arduino.h"
#include "Wire.h"
uint32_t clockMs=0;
SerialMock Serial;
TwoWire Wire;
#include "../Attenuator_Bargraph.ino"
void cmd(const char *s) { char b[100]; strcpy(b,s); execute(b); }
void tick(uint32_t dt=1000) { clockMs+=dt; animate(); }
int main() {
 setup(); assert(ready && mode==SELF_TEST);
 tick(); assert(logicalImage==0xfffffffUL);
 tick(); assert(logicalImage==0);
 tick(); assert(mode==DEMO);
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
 Wire.connected=true; cmd("scan"); assert(ready && logicalImage==0);
 std::cout << "PASS: raw RAM, discovery coverage, command bounds, inversion, animations, wraparound, parser overflow, I2C failure/recovery\n";
}

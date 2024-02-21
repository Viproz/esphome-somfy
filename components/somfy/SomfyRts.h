#ifndef somfyrts_h
#define somfyrts_h
#include <Arduino.h>
#include <Preferences.h>

#define SYMBOL 640
#define HAUT 0x2
#define STOP 0x1
#define BAS 0x4
#define PROG 0x8

#ifndef REMOTE_TX_PIN
#define REMOTE_TX_PIN 2
#endif

class SomfyRts {
private:
    bool _debug;
    uint32_t _remoteId;
    unsigned char _frame[7];
    char checksum;
    uint16_t rollingCode;
    uint16_t savedRollingCode;
    void _writeRemoteRollingCode(uint16_t code);

public:
    SomfyRts(uint32_t remoteID, bool debug=false);
    void init();
    void sendCommandUp();
    void sendCommandDown();
    void sendCommandStop();
    void sendCommandProg();
    void sendCommandProgGrail();
    void buildFrame(unsigned char* frame, unsigned char button);
    void sendCommand(unsigned char* frame, unsigned char sync);
    uint16_t readRemoteRollingCode();
    String getConfigFilename();
};

#endif

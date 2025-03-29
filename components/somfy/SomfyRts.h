#ifndef somfyrts_h
#define somfyrts_h
#include <Arduino.h>
#include <Preferences.h>
#include <queue>

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
    std::queue<unsigned char>* _bufferQueue;
    byte _bufferByte;
    int _bufferBit;
    char checksum;
    uint16_t rollingCode;
    uint16_t savedRollingCode;
    void _writeRemoteRollingCode(uint16_t code);
    void setNextBufferBit(char state, int nbBits = 1);
    void completeBuffer();

public:
    SomfyRts(uint32_t remoteID, std::queue<unsigned char>* bufferQueue);
    void init();
    void sendCommandUp();
    void sendCommandDown();
    void sendCommandStop();
    void sendCommandProg(int nbCmd = 2);
    void sendCommandProgGrail();
    void buildFrame(unsigned char* frame, unsigned char button);
    void sendCommand(unsigned char* frame, unsigned char sync);
    uint16_t readRemoteRollingCode();
    String getConfigFilename();
};

#endif

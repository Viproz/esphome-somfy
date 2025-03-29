#include "SomfyRts.h"
#include <Preferences.h>

SomfyRts::SomfyRts(uint32_t remoteID, std::queue<unsigned char>* bufferQueue)
{
    _remoteId = remoteID;
    _bufferQueue = bufferQueue;
    _bufferBit = 0;
}

void SomfyRts::init()
{
    pinMode(REMOTE_TX_PIN, OUTPUT);
    digitalWrite(REMOTE_TX_PIN, LOW);

    rollingCode = readRemoteRollingCode();
    savedRollingCode = rollingCode;

    if (Serial)
    {
        Serial.print("Simulated remote number : ");
        Serial.println(_remoteId, HEX);
        Serial.print("Current rolling code    : ");
        Serial.println(rollingCode);
    }
}

void SomfyRts::buildFrame(unsigned char *frame, unsigned char button)
{
    frame[0] = 0xA7;            // Encryption key. Doesn't matter much
    frame[1] = button << 4;     // Which button did  you press? The 4 LSB will be the checksum
    frame[2] = rollingCode >> 8;       // Rolling code (big endian)
    frame[3] = rollingCode;            // Rolling code
    frame[4] = _remoteId >> 16; // Remote address
    frame[5] = _remoteId >> 8;  // Remote address
    frame[6] = _remoteId;       // Remote address

    if (Serial)
    {
        Serial.print("Frame         : ");
        for (byte i = 0; i < 7; i++)
        {
            if (frame[i] >> 4 == 0)
            {                      // Displays leading zero in case the most significant
                Serial.print("0"); // nibble is a 0.
            }
            Serial.print(frame[i], HEX);
            Serial.print(" ");
        }
    }

    // Checksum calculation: a XOR of all the nibbles
    checksum = 0;
    for (byte i = 0; i < 7; i++)
    {
        checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
    }
    checksum &= 0b1111; // We keep the last 4 bits only

    // Checksum integration
    frame[1] |= checksum; //  If a XOR of all the nibbles is equal to 0, the blinds will
                          // consider the checksum ok.

    if (Serial)
    {
        Serial.println("");
        Serial.print("With checksum : ");
        for (byte i = 0; i < 7; i++)
        {
            if (frame[i] >> 4 == 0)
            {
                Serial.print("0");
            }
            Serial.print(frame[i], HEX);
            Serial.print(" ");
        }
    }

    // Obfuscation: a XOR of all the bytes
    for (byte i = 1; i < 7; i++)
    {
        frame[i] ^= frame[i - 1];
    }

    if (Serial)
    {
        Serial.println("");
        Serial.print("Obfuscated    : ");
        for (byte i = 0; i < 7; i++)
        {
            if (frame[i] >> 4 == 0)
            {
                Serial.print("0");
            }
            Serial.print(frame[i], HEX);
            Serial.print(" ");
        }
        Serial.println("");
        Serial.print("Rolling Code  : ");
        Serial.println(rollingCode);
    }

    // We store the value of the rolling code in the FS if the saved value is too low
    if(savedRollingCode <= rollingCode) {
        // Write the rolling code of the future, do it once every 5th time to save on write cycles
        _writeRemoteRollingCode(rollingCode + 5);
        savedRollingCode = rollingCode + 5;
    }

    // Increment the rolling code
    ++rollingCode;
    
}

void SomfyRts::setNextBufferBit(char state, int nbBits)
{
    for (int i = 0; i < nbBits; ++i) {
        if (state == 0) {
            _bufferByte = _bufferByte & ~(1 << (7 - _bufferBit % 8));
        } else {
            _bufferByte = _bufferByte | (1 << (7 - _bufferBit % 8));
        }
        ++_bufferBit;

        if (_bufferBit >= 8) {
            // Add the byte to the queue
            _bufferQueue->push(_bufferByte);
            _bufferByte = 0;
            _bufferBit = 0;
        }
    }
}

void SomfyRts::completeBuffer()
{
    // Send the rest of the byte however full it is, it guarantees to finish with a 0
    _bufferQueue->push(_bufferByte);
    _bufferByte = 0;
    _bufferBit = 0;
}

void SomfyRts::sendCommand(unsigned char *frame, unsigned char sync)
{
    // Total frame is a bit more than 210105 us = 210 ms
    if (sync == 2)
    { // Only with the first frame.
        // Wake-up pulse & Silence
        setNextBufferBit(1, 16);
        setNextBufferBit(0, 148);
    }

    // Hardware sync: two sync for the first frame, seven for the following ones.
    for (int i = 0; i < sync; i++) {
        setNextBufferBit(1, 4);
        
        setNextBufferBit(0, 4);
    }

    // Software sync
    setNextBufferBit(1, 8);
    setNextBufferBit(0);

    // Data: bits are sent one by one, starting with the MSB.
    for (byte i = 0; i < 56; i++)
    {
        if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1)
        {
            setNextBufferBit(0);
            setNextBufferBit(1);
        }
        else
        {
            setNextBufferBit(1);
            setNextBufferBit(0);
        }
    }

    // Inter-frame silence
    setNextBufferBit(0, 51);
}

void SomfyRts::sendCommandUp()
{
    buildFrame(_frame, HAUT);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
    completeBuffer();
}

void SomfyRts::sendCommandDown()
{
    buildFrame(_frame, BAS);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
    completeBuffer();
}

void SomfyRts::sendCommandStop()
{
    buildFrame(_frame, STOP);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
    completeBuffer();
}

void SomfyRts::sendCommandProg(int nbCmd)
{
    buildFrame(_frame, PROG);
    sendCommand(_frame, 2);
    for (int i = 0; i < nbCmd; i++)
    {
        sendCommand(_frame, 7);
    }
    completeBuffer();
}

// support for grail is limited: I can only pair one remote
void SomfyRts::sendCommandProgGrail()
{
    // No clue what grail is
    sendCommandProg(35);
    
    // Wait 4s
    setNextBufferBit(0, 6250);

    sendCommandProg();
    completeBuffer();
}

uint16_t SomfyRts::readRemoteRollingCode()
{
    uint16_t code = 0;
    Preferences preferences;
    // Open the project namespace as read only
    preferences.begin("SomfyCover", true);

    // Read the key, if it doesn't exist return 0
    code = preferences.getUShort(getConfigFilename().c_str(), 0);

    preferences.end();
    return code;
}

void SomfyRts::_writeRemoteRollingCode(uint16_t code)
{
    Preferences preferences;
    // Open the project namespace as read write
    preferences.begin("SomfyCover", false);

    // Save the code
    preferences.putUShort(getConfigFilename().c_str(), code);
    
    preferences.end();
}

String SomfyRts::getConfigFilename()
{
    return String(_remoteId);
}

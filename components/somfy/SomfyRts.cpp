#include "SomfyRts.h"
#include <Preferences.h>

SomfyRts::SomfyRts(uint32_t remoteID, bool debug)
{
    _debug = debug;
    _remoteId = remoteID;
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

void SomfyRts::sendCommand(unsigned char *frame, unsigned char sync)
{
    // Total frame is a bit more than 210105 us = 210 ms
    if (sync == 2)
    { // Only with the first frame.
        // Wake-up pulse & Silence
        digitalWrite(REMOTE_TX_PIN, HIGH);
        delayMicroseconds(9415);
        digitalWrite(REMOTE_TX_PIN, LOW);
        delayMicroseconds(89565);
    }

    // Hardware sync: two sync for the first frame, seven for the following ones.
    for (int i = 0; i < sync; i++)
    {
        digitalWrite(REMOTE_TX_PIN, HIGH);
        delayMicroseconds(4 * SYMBOL);
        digitalWrite(REMOTE_TX_PIN, LOW);
        delayMicroseconds(4 * SYMBOL);
    }

    // Software sync
    digitalWrite(REMOTE_TX_PIN, HIGH);
    delayMicroseconds(4550);
    digitalWrite(REMOTE_TX_PIN, LOW);
    delayMicroseconds(SYMBOL);

    // Data: bits are sent one by one, starting with the MSB.
    for (byte i = 0; i < 56; i++)
    {
        if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1)
        {
            digitalWrite(REMOTE_TX_PIN, LOW);
            delayMicroseconds(SYMBOL);
            // PORTD ^= 1<<5;
            digitalWrite(REMOTE_TX_PIN, HIGH);
            delayMicroseconds(SYMBOL);
        }
        else
        {
            digitalWrite(REMOTE_TX_PIN, HIGH);
            delayMicroseconds(SYMBOL);
            // PORTD ^= 1<<5;
            digitalWrite(REMOTE_TX_PIN, LOW);
            delayMicroseconds(SYMBOL);
        }
    }

    digitalWrite(REMOTE_TX_PIN, LOW);
    delayMicroseconds(30415); // Inter-frame silence
}

void SomfyRts::sendCommandUp()
{
    buildFrame(_frame, HAUT);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
}

void SomfyRts::sendCommandDown()
{
    buildFrame(_frame, BAS);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
}

void SomfyRts::sendCommandStop()
{
    buildFrame(_frame, STOP);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
}

void SomfyRts::sendCommandProg()
{
    buildFrame(_frame, PROG);
    sendCommand(_frame, 2);
    for (int i = 0; i < 2; i++)
    {
        sendCommand(_frame, 7);
    }
}

// support for grail is limited: I can only pair one remote
void SomfyRts::sendCommandProgGrail()
{
    buildFrame(_frame, PROG);
    sendCommand(_frame, 2);
    for (int i = 0; i < 35; i++)
    {
        sendCommand(_frame, 7);
    }
    for (int i = 0; i < 40; i++)
    {
        delayMicroseconds(100000);
    }
    sendCommandProg();
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

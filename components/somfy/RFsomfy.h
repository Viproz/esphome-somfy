#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/cover/cover.h"
#include "SomfyRts.h"
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace esphome {
namespace somfy {

// cmd 11 - program mode
// cmd 16 - porgram mode for grail curtains
// cmd 21 - delete rolling code file
// cmd 41 - List files
// cmd 51 - Test filesystem.
// cmd 61 - Format filesystem and test.
// cmd 71 - Show actual rolling code
// cmd 81 - Get all rolling code
// cmd 85 - Write new rolling codes

#define STATUS_LED_PIN 22
#define REMOTE_TX_PIN 2
#define REMOTE_FIRST_ADDR 0x121311 // Starting number for remote indexes


char* const string2char(String command)
{
    if (command.length() != 0)
    {
        char *p = const_cast<char *>(command.c_str());
        return p;
    }
    return const_cast<char *>("");
}

String file_path(int remoteId)
{
    String path = "/data_remote_";
    path += remoteId;
    path += ".txt";
    return path;
}


void writeCode2file(int remoteId, uint16_t code)
{
    SPIFFS.begin();
    Serial.println("Writing config");
    String arq = file_path(remoteId);
    File f = SPIFFS.open(arq, "w");
    if (f)
    {
        f.println(code);
        f.close();
        ESP_LOGI("RFsomfy.h", "Wrote code: %d", code);
    }
    else
    {
        ESP_LOGW("RFsomfy.h", "File creation failed");
    }
    SPIFFS.end();
}

class RFsomfy : public Component, public cover::Cover
{

private:
    int index;
    int remoteId = -1;

    SomfyRts* rtsDevice;

public:
    void set_code(const uint16_t code)
    {
        writeCode2file(remoteId, code);
    }

    void setCoverID(int coverID)
    {
        index = coverID;
        remoteId = REMOTE_FIRST_ADDR + coverID;
    }

    void setup() override
    {
        rtsDevice = new SomfyRts(remoteId);

        ESP_LOGD("RFsomfy.h", "Cover %d", index);

        // This will be called by App.setup()
        ESP_LOGD("RFsomfy.h", "Starting Device");
        Serial.begin(115200);
        Serial.println("Initialize remote device");
        pinMode(STATUS_LED_PIN, OUTPUT);
        digitalWrite(STATUS_LED_PIN, HIGH);
        ESP_LOGD("RFsomfy.h", "Somfy ESPHome Cover v1.00");
        ESP_LOGD("RFsomfy.h", "Initialize remote device");
        
        rtsDevice->init();
        

        digitalWrite(STATUS_LED_PIN, LOW);

        if (ELECHOUSE_cc1101.getCC1101()) {
            ESP_LOGD("RFsomfy.h", "Communication established with the CC1101 module");
        } else {
            ESP_LOGD("RFsomfy.h", "Error: Could not establish communication with the CC1101 module");
        }

        ELECHOUSE_cc1101.Init();
        ELECHOUSE_cc1101.setMHZ(433.42);
    }

    // delete rolling code . 0....n
    void delete_code()
    {
        SPIFFS.begin();
        String path = file_path(remoteId);

        SPIFFS.remove(path);

        ESP_LOGD("RFsomfy.h", "Deleted remote %i", remoteId);
        SPIFFS.end();
    }

    cover::CoverTraits get_traits() override
    {
        auto traits = cover::CoverTraits();
        traits.set_is_assumed_state(false);
        traits.set_supports_position(true);
        traits.set_supports_tilt(true); // to send other commands
        return traits;
    }

    void control(const cover::CoverCall &call) override
    {
        // This will be called every time the user requests a state change.

        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(50);

        ESP_LOGW("RFsomfy.h", "Using remote %d", REMOTE_FIRST_ADDR + index);
        ESP_LOGW("RFsomfy.h", "Remoteid %d", remoteId);
        ESP_LOGW("RFsomfy.h", "index %d", index);

        if (call.get_position().has_value()) {
            float pos = *call.get_position();
            // Write pos (range 0-1) to cover
            // ...
            int ppos = pos * 100;
            ESP_LOGD("RFsomfy.h", "get_position is: %d", ppos);

            if (ppos == 0) {
                ESP_LOGD("RFsomfy.h", "POS 0");
                Serial.println("* Command Down");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandDown();
                
                ELECHOUSE_cc1101.setSidle();
                pos = 0.01;
            } else if (ppos == 100) {
                ESP_LOGD("RFsomfy.h", "POS 100");
                Serial.println("* Command UP");
                    ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandUp();
                
                ELECHOUSE_cc1101.setSidle();
                pos = 0.99;
            } else {
                // In between position, set it to saved position
                ESP_LOGD("RFsomfy.h", "POS 50");
                Serial.println("* Command MY");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandStop();
                
                ELECHOUSE_cc1101.setSidle();
                pos = 0.5;
            }

            // Publish new state
            this->position = pos;
            this->publish_state();
        } else if (call.get_stop()) {
            // User requested cover stop
            ESP_LOGD("RFsomfy", "get_stop");
            ELECHOUSE_cc1101.SetTx();

            rtsDevice->sendCommandStop();
            
            ELECHOUSE_cc1101.setSidle();
        } else if (call.get_tilt().has_value()) {
            // Tilt is only for debug/programation
            auto tpos = *call.get_tilt();
            int xpos = tpos * 100;
            ESP_LOGI("RFsomfy.h", "Command tilt xpos: %d", xpos);

            if (xpos == 11)
            {
                ESP_LOGD("RFsomfy.h", "program mode");
                digitalWrite(STATUS_LED_PIN, HIGH);
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandProg();
                
                ELECHOUSE_cc1101.setSidle();
                delay(1000);
            }
            if (xpos == 16)
            {
                ESP_LOGD("RFsomfy.h", "program mode - grail");
                digitalWrite(STATUS_LED_PIN, HIGH);
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandProgGrail();
                
                ELECHOUSE_cc1101.setSidle();
                delay(1000);
            }
            if (xpos == 21)
            {
                ESP_LOGD("RFsomfy.h", "delete file");
                digitalWrite(STATUS_LED_PIN, HIGH);
                delete_code();
                delay(1000);
            }

            if (xpos == 61)
            {
                ESP_LOGD("RFsomfy.h", "61 mode");

                // Begin and try to format on fail
                bool success = SPIFFS.begin(true);
                if(success) {
                    ESP_LOGW("RFsomfy.h", "Begin success");
                } else {
                    ESP_LOGW("RFsomfy.h", "Begin fail");
                }

                if (!SPIFFS.exists("/formatComplete.txt"))
                {
                    // Serial.println("Please wait 30 secs for SPIFFS to be formatted");
                    // ESP_LOGW("RFsomfy.h", "Please wait 30 s");

                    // success = SPIFFS.format(); // This seems to bug out and badly format the space
                    // delay(30000);

                    if(success) {
                        ESP_LOGW("RFsomfy.h", "Spiffs formatted");
                    } else {
                        ESP_LOGW("RFsomfy.h", "Error while Spiffs formatting");
                    }

                    File f = SPIFFS.open("/formatComplete.txt", "w");
                    if (!f) {
                        ESP_LOGW("RFsomfy.h", "file open failed");
                    } else {
                        f.println("Format Complete");
                        ESP_LOGW("RFsomfy.h", "Format Complete");
                    }
                    f.close();

                } else {
                    ESP_LOGW("RFsomfy.h", "SPIFFS is formatted. Moving along...");
                }
                SPIFFS.end();
            }

            // Debug commands
            if (xpos == 90) {
                setup();
            }

            if (xpos == 97) {
                ELECHOUSE_cc1101.SetTx();
            }

            if (xpos == 98) {
                ELECHOUSE_cc1101.setSidle();
            }

            if (xpos == 99) {
                digitalWrite(2, HIGH);
            }

            if (xpos == 100) {
                digitalWrite(2, LOW);
            }

            // Don't publish
        }

        digitalWrite(STATUS_LED_PIN, LOW);
        delay(50);
    }
};

}  // namespace somfy
}  // namespace esphome
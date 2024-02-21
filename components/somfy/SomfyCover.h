#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/cover/cover.h"
#include "SomfyRts.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace esphome {
namespace somfy {

// cmd 11 - program mode
// cmd 16 - porgram mode for grail curtains
// cmd 21 - delete rolling code file
// cmd 61 - Clears all Preferences set
// cmd 90 - Re-run the setup member
// cmd 97 - Set the CC1101 module to TX mode
// cmd 98 - Set the CC1101 module to idle
// cmd 99 - Set the transmit pin to HIGH
// cmd 100 - Set the transmit pin to LOW

#define REMOTE_TX_PIN 2
#define REMOTE_FIRST_ADDR 0x121311 // Starting number for remote indexes


String file_path(int remoteId)
{
    String path = "/data_remote_";
    path += remoteId;
    path += ".txt";
    return path;
}

class SomfyCover : public Component, public cover::Cover
{

private:
    int index;
    int remoteId = -1;

    SomfyRts* rtsDevice;

public:
    void setCoverID(int coverID)
    {
        index = coverID;
        remoteId = REMOTE_FIRST_ADDR + coverID;
    }

    void setup() override
    {
        rtsDevice = new SomfyRts(remoteId);

        ESP_LOGD("SomfyCover.h", "Cover %d", index);

        // This will be called by App.setup()
        ESP_LOGD("SomfyCover.h", "Starting Device");
        Serial.begin(115200);
        Serial.println("Initialize remote device");
        ESP_LOGD("SomfyCover.h", "Somfy ESPHome Cover v1.00");
        ESP_LOGD("SomfyCover.h", "Initialize remote device");

        rtsDevice->init();


        if (ELECHOUSE_cc1101.getCC1101()) {
            ESP_LOGD("SomfyCover.h", "Communication established with the CC1101 module");
        }
        else {
            ESP_LOGD("SomfyCover.h", "Error: Could not establish communication with the CC1101 module");
        }

        ELECHOUSE_cc1101.Init();
        ELECHOUSE_cc1101.setMHZ(433.42);
    }

    // delete rolling code . 0....n
    void delete_code()
    {
        Preferences preferences;
        preferences.begin("SomfyCover", false);
        const char* path = file_path(remoteId).c_str();

        preferences.remove(path);

        ESP_LOGD("SomfyCover.h", "Deleted remote %i", remoteId);
        preferences.end();
    }

    cover::CoverTraits get_traits() override
    {
        auto traits = cover::CoverTraits();
        traits.set_is_assumed_state(false);
        traits.set_supports_position(true);
        traits.set_supports_stop(true); // Middle button of the remote
        traits.set_supports_tilt(true); // to send other commands
        return traits;
    }

    void control(const cover::CoverCall& call) override
    {
        // This will be called every time the user requests a state change.

        ESP_LOGW("SomfyCover.h", "Using remote %d", REMOTE_FIRST_ADDR + index);
        ESP_LOGW("SomfyCover.h", "Remoteid %d", remoteId);
        ESP_LOGW("SomfyCover.h", "index %d", index);

        if (call.get_position().has_value()) {
            float pos = *call.get_position();
            // Write pos (range 0-1) to cover
            // ...
            int ppos = pos * 100;
            ESP_LOGD("SomfyCover.h", "get_position is: %d", ppos);

            if (ppos == 0) {
                ESP_LOGD("SomfyCover.h", "POS 0");
                Serial.println("* Command Down");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandDown();

                ELECHOUSE_cc1101.setSidle();
                pos = 0.01;
            }
            else if (ppos == 100) {
                ESP_LOGD("SomfyCover.h", "POS 100");
                Serial.println("* Command UP");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandUp();

                ELECHOUSE_cc1101.setSidle();
                pos = 0.99;
            }
            else {
                // In between position, set it to saved position
                ESP_LOGD("SomfyCover.h", "POS 50");
                Serial.println("* Command MY");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandStop();

                ELECHOUSE_cc1101.setSidle();
                pos = 0.5;
            }

            // Publish new state
            this->position = pos;
            this->publish_state();
        }
        else if (call.get_stop()) {
            // User requested cover stop
            ESP_LOGD("SomfyCover", "get_stop");
            ELECHOUSE_cc1101.SetTx();

            rtsDevice->sendCommandStop();

            ELECHOUSE_cc1101.setSidle();
        }
        else if (call.get_tilt().has_value()) {
            // Tilt is only for debug/programation
            auto tpos = *call.get_tilt();
            int xpos = tpos * 100;
            ESP_LOGI("SomfyCover.h", "Command tilt xpos: %d", xpos);

            if (xpos == 0)
            {
                String txt = "Current rolling code is ";
                txt += rtsDevice->readRemoteRollingCode();
                txt += ".";
                ESP_LOGD("SomfyCover.h", txt.c_str());
            }
            if (xpos == 11)
            {
                ESP_LOGD("SomfyCover.h", "program mode");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandProg();

                ELECHOUSE_cc1101.setSidle();
                delay(1000);
            }
            if (xpos == 16)
            {
                ESP_LOGD("SomfyCover.h", "program mode - grail");
                ELECHOUSE_cc1101.SetTx();

                rtsDevice->sendCommandProgGrail();

                ELECHOUSE_cc1101.setSidle();
                delay(1000);
            }
            if (xpos == 21)
            {
                ESP_LOGD("SomfyCover.h", "delete file");
                delete_code();
                delay(1000);
            }

            if (xpos == 61)
            {
                ESP_LOGD("SomfyCover.h", "Clearing values in Preference library.");

                Preferences preferences;
                bool success = preferences.begin("SomfyCover", false);
                if (success) {
                    ESP_LOGW("SomfyCover.h", "Begin success");
                }
                else {
                    ESP_LOGW("SomfyCover.h", "Begin fail");
                }

                preferences.clear();

                preferences.end();
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
    }
};

}  // namespace somfy
}  // namespace esphome
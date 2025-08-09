#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/cover/cover.h"
#include "SomfyRts.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <nvs_flash.h>
#include <queue>

namespace esphome {
namespace somfy {

// cmd 0 - Prints the current rolling code
// cmd 11 - Program mode
// cmd 16 - Program mode for grail curtains
// cmd 21 - Delete rolling code file
// cmd 50 - Long Program mode
// cmd 61 - Clears all Preferences set
// cmd 90 - Re-run the setup member
// cmd 97 - Set the CC1101 module to TX mode
// cmd 98 - Set the CC1101 module to idle
// cmd 99 - Set the transmit pin to HIGH
// cmd 100 - Set the transmit pin to LOW

#define REMOTE_TX_PIN 2
#define REMOTE_FIRST_ADDR 0x121311 // Starting number for remote indexes

class SomfyCover : public Component, public cover::Cover
{

private:
    int index;
    int remoteId = -1;
    unsigned char _buffer[64];

    SomfyRts* rtsDevice;

public:
    static ELECHOUSE_CC1101 cc1101;
    static std::queue<unsigned char> _bufferQueue;

    void setCoverID(int coverID)
    {
        index = coverID;
        remoteId = REMOTE_FIRST_ADDR + coverID;
    }

    void setup() override
    {
        rtsDevice = new SomfyRts(remoteId, &_bufferQueue);

        ESP_LOGD("SomfyCover.h", "Cover %d", index);

        // This will be called by App.setup()
        ESP_LOGD("SomfyCover.h", "Starting Device");
        Serial.begin(115200);
        Serial.println("Initialize remote device");
        ESP_LOGD("SomfyCover.h", "Somfy ESPHome Cover v1.00");
        ESP_LOGD("SomfyCover.h", "Initialize remote device");

        // Open the preference memory to create the space if necessary
        Preferences preferences;
        preferences.begin("SomfyCover", false);
        preferences.end();

        rtsDevice->init();


        if (cc1101.getCC1101()) {
            ESP_LOGD("SomfyCover.h", "Communication established with the CC1101 module");
        }
        else {
            ESP_LOGD("SomfyCover.h", "Error: Could not establish communication with the CC1101 module");
        }

        cc1101.Init();
        
        cc1101.setCCMode(1);
        cc1101.setMHZ(433.42);
        cc1101.setModulation(2); // ASK/OOK
        cc1101.setDRate(1.557);  // Set the Data Rate in kBaud. Value from 0.02 to 1621.83.
        cc1101.setCrc(0);
        cc1101.setSyncMode(0); 

        cc1101.setChannel(0);         // Set the Channelnumber from 0 to 255. Default is cahnnel 0.

        //cc1101.setPA(10);             // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
        cc1101.setSyncWord(0, 0); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
        cc1101.setAdrChk(0);          // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
        cc1101.setAddr(0);            // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
        cc1101.setWhiteData(0);       // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
        cc1101.setPktFormat(0);       // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
        cc1101.setLengthConfig(2);    // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
        cc1101.setPacketLength(0);    // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.

        cc1101.setManchester(0);      // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
        cc1101.setFEC(0);             // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
        cc1101.setPRE(0);             // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
        cc1101.setPQT(0);             // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
        cc1101.setAppendStatus(0);    // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.


        cc1101.SpiStrobe(CC1101_SIDLE);
        // Flush TX FIFO
        cc1101.SpiStrobe(CC1101_SFTX);
    }

    void loop() override
    {
        // Get the number of free bytes in the FIFO and status of the chip
        byte status = cc1101.SpiStrobe(CC1101_SNOP | 0x40);
        
        // See the state
        byte state = status >> 4 & 0x7;

        if (_bufferQueue.size() > 0) {
            ESP_LOGD("SomfyCover.h", "Status: 0x%02X, bytes left: %d", status, _bufferQueue.size());

            int bytesToWrite = _min(_bufferQueue.size(), status & 0x0F);

            if (status & 0x0F == 0x0F && state == 0) {
                // Start of transmission, TX FIFO is supposed to be empty transmit 64 bytes
                bytesToWrite = _min(_bufferQueue.size(), 64);
            }

            if (bytesToWrite == 0) {
                if (state == 4) {
                    // Chip is calibrating
                    ESP_LOGD("SomfyCover.h", "Calibration in progress");
                    delayMicroseconds(500);
                } else if (state != 2) {
                    // Something very wrong happened, flush everything
                    ESP_LOGD("SomfyCover.h", "Something wrong occured, CC1101 not in transmit but FIFO full, flush FIFO");
                    cc1101.SpiStrobe(CC1101_SIDLE);
                    cc1101.SpiStrobe(CC1101_SFTX);
                }

                // Nothing to write
                return;
            }

            // Add bits to the FIFO
            for (int i = 0; i < bytesToWrite; i++) {
                _buffer[i] = _bufferQueue.front();
                _bufferQueue.pop();
            }
            byte resp = cc1101.SpiWriteBurstReg(CC1101_TXFIFO, _buffer, bytesToWrite);
            ESP_LOGD("SomfyCover.h", "Written %d bytes to FIFO, resp 0x%02X", bytesToWrite, resp);

            if (state == 7) {
                // TX Underflow
                ESP_LOGD("SomfyCover.h", "TX Underflow occured");
            }

            if (state != 2) {
                ESP_LOGD("SomfyCover.h", "Switch to transmit");
                cc1101.SpiStrobe(CC1101_SIDLE);
                cc1101.SpiStrobe(CC1101_STX); //start send
            }
        } else if(state == 7) {
            // Done writing, switch to IDLE
            ESP_LOGD("SomfyCover.h", "Switch to IDLE");
            cc1101.SpiStrobe(CC1101_SIDLE); // Exit RX / TX, turn off frequency synthesizer and exit
            cc1101.SpiStrobe(CC1101_SFTX);
        }
    }

    // delete rolling code . 0....n
    void delete_code()
    {
        Preferences preferences;
        preferences.begin("SomfyCover", false);
        const char* path = rtsDevice->getConfigFilename().c_str();

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

                rtsDevice->sendCommandDown();

                pos = 0.01;
            } else if (ppos == 100) {
                ESP_LOGD("SomfyCover.h", "POS 100");
                Serial.println("* Command UP");

                rtsDevice->sendCommandUp();

                pos = 0.99;
            } else {
                // In between position, set it to saved position
                ESP_LOGD("SomfyCover.h", "POS 50");
                Serial.println("* Command MY");

                rtsDevice->sendCommandStop();

                pos = 0.5;
            }

            // Publish new state
            this->position = pos;
            this->publish_state();
        }
        else if (call.get_stop()) {
            // User requested cover stop
            ESP_LOGD("SomfyCover", "get_stop");

            rtsDevice->sendCommandStop();

        }
        else if (call.get_tilt().has_value()) {
            // Tilt is only for debug/programation
            auto tpos = *call.get_tilt();
            int xpos = tpos * 100;
            Preferences preferences;
            bool success;
            int ret;
            ESP_LOGI("SomfyCover.h", "Command tilt xpos: %d", xpos);

            switch (xpos)
            {
            case 0:
                ESP_LOGD("SomfyCover.h", "Current rolling code is %d.", rtsDevice->readRemoteRollingCode());
                break;

            case 11:
                ESP_LOGD("SomfyCover.h", "program mode");

                rtsDevice->sendCommandProg();
                break;

            case 16:
                ESP_LOGD("SomfyCover.h", "program mode - grail");

                rtsDevice->sendCommandProgGrail();
                break;

            case 21:
                ESP_LOGD("SomfyCover.h", "delete file");
                delete_code();
                break;

            case 50:
                ESP_LOGD("SomfyCover.h", "long program mode");
                rtsDevice->sendCommandProg(20);
                break;

            case 61:
                ESP_LOGD("SomfyCover.h", "Clearing all values in Preference library.");

                nvs_flash_erase(); // erase the NVS partition and...
                nvs_flash_init(); // initialize the NVS partition.

                success = preferences.begin("SomfyCover", false);
                if (success) {
                    ESP_LOGW("SomfyCover.h", "Begin success");
                } else {
                    ESP_LOGW("SomfyCover.h", "Begin fail");
                }

                ret = preferences.putUShort("test", 20);
                if (ret == 0) {
                    ESP_LOGW("SomfyCover.h", "Error while test-writing.");
                } else {
                    ESP_LOGW("SomfyCover.h", "Memory write success.");
                }

                preferences.end();
                break;

            // Debug commands
            case 90:
                setup();
                break;

            case 97:
                cc1101.SetTx();
                break;
            
            case 98:
                cc1101.setSidle();
                break;

            case 99:
                digitalWrite(2, HIGH);
                break;

            case 100:
                digitalWrite(2, LOW);
                break;
            
            default:
                break;
            }

            // Don't publish
        }
    }
};

ELECHOUSE_CC1101 SomfyCover::cc1101;
std::queue<unsigned char> SomfyCover::_bufferQueue;


}  // namespace somfy
}  // namespace esphome

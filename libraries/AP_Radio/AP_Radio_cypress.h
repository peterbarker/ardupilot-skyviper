/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

/*
  AP_Radio implementation for Cypress 2.4GHz radio. 

  With thanks to the SuperBitRF project
  See http://wiki.paparazziuav.org/wiki/SuperbitRF

  This implementation uses the DSMX protocol on a CYRF6936
 */

#include "AP_Radio_backend.h"
#include <nuttx/arch.h>
#include <systemlib/systemlib.h>
#include <drivers/drv_hrt.h>

class AP_Radio_cypress : public AP_Radio_backend
{
public:
    AP_Radio_cypress(AP_Radio &radio);
    
    // init - initialise radio
    bool init(void) override;

    // rest radio
    bool reset(void) override;
    
    // send a packet
    bool send(const uint8_t *pkt, uint16_t len) override;

    // start bind process as a receiver
    void start_recv_bind(void) override;

    // return time in microseconds of last received R/C packet
    uint32_t last_recv_us(void) override;

    // return number of input channels
    uint8_t num_channels(void) override;

    // return current PWM of a channel
    uint16_t read(uint8_t chan) override;

    // get radio statistics structure
    const AP_Radio::stats &get_stats(void) override;
    
private:
    AP_HAL::OwnPtr<AP_HAL::SPIDevice> dev;
    static AP_Radio_cypress *radio_instance;

    void radio_init(void);
    
    void dump_registers(uint8_t n);

    void force_initial_state(void);
    void set_channel(uint8_t channel);
    uint8_t read_status_debounced(uint8_t adr);
    
    uint8_t read_register(uint8_t reg);
    void write_register(uint8_t reg, uint8_t value);
    void write_multiple(uint8_t reg, uint8_t n, const uint8_t *data);

    enum {
        STATE_RECV,
        STATE_BIND,
        STATE_SEND
    } state;
    
    struct config {
        uint8_t reg;
        uint8_t value;
    };
    static const uint8_t pn_codes[5][9][8];
    static const uint8_t pn_bind[];
    static const config cyrf_config[];
    static const config cyrf_bind_config[];
    static const config cyrf_transfer_config[];
    
    sem_t irq_sem;

    struct hrt_call wait_call;

    void radio_set_config(const struct config *config, uint8_t size);

    void start_receive(void);
    
    // main IRQ handler
    void irq_handler(void);

    // IRQ handler for packet receive
    void irq_handler_recv(uint8_t rx_status);

    // handle timeout IRQ
    void irq_timeout(void);
    
    // trampoline functions to take us from static IRQ function to class functions
    static int irq_radio_trampoline(int irq, void *context);
    static int irq_timeout_trampoline(int irq, void *context);

    static const uint8_t max_channels = 16;

    AP_Radio::stats stats;
    
    // dsm config data and status
    struct {
        uint8_t channels[23];
        uint8_t mfg_id[4] {0xA5, 0xA7, 0x55, 0x0C};
        uint8_t current_channel;
        uint8_t current_rf_channel;
        uint16_t crc_seed;
        uint8_t sop_col;
        uint8_t data_col;
        bool is_dsm2 = false;
        uint8_t last_sop_code[8];
        uint8_t last_data_code[16];

        uint32_t receive_start_us;
        uint32_t receive_timeout_usec;

        uint32_t last_recv_us;
        uint32_t last_recv_chan;
        uint32_t last_crc_seed;
        uint32_t last_chan_change_us;
        uint8_t num_channels;
        uint16_t pwm_channels[max_channels];
    } dsm;
    
    // DSM specific functions
    void dsm_set_channel(uint8_t channel, bool is_dsm2, uint8_t sop_col, uint8_t data_col, uint16_t crc_seed);

    // generate DSMX channels
    void dsm_generate_channels_dsmx(uint8_t mfg_id[4], uint8_t channels[23]);

    // setup for DSMX transfers
    void dsm_setup_transfer_dsmx(void);

    // move to next DSM channel
    void dsm_set_next_channel(void);

    void dsm_choose_channel(void);
    
    // parse DSM channels from a packet
    void parse_dsm_channels(const uint8_t *data);

    // process an incoming packet
    void process_packet(const uint8_t *pkt, uint8_t len);

    // process an incoming bind packet
    void process_bind(const uint8_t *pkt, uint8_t len);
    
};


#pragma once
#include <stdio.h>
#include <stdint.h>

/*
This head file is for Intan RHS2116.
Datasheet can be found here: https://intantech.com/files/Intan_RHS2116_datasheet.pdf
*/

/* Misc Constants */
#define STIM_EN_A     0xAAAA // Magic number. See Specsheet Pg.38
#define STIM_EN_B     0x00FF // Magic number. See Specsheet Pg.38

#define reg_val_t uint16_t

/* On-Chip Read-Only Registers. Datasheet Pg.43*/
#define RO_REG_COMPANY_DESIGNATION_P1 251
#define RO_REG_COMPANY_DESIGNATION_P2 252
#define RO_REG_COMPANY_DESIGNATION_P3 253

#define RO_REG_CHANNEL_TOTAL_CHIP_REV 254
typedef struct __attribute__ ((__packed__)) {
    uint8_t channel_total : 8; // 1st (lowerest) 8 bit
    uint8_t chip_rev : 8;      // 2nd 8 bit
} reg_channel_tota_chip_rev_t;

#define RO_REG_CHIP_ID                255
typedef struct __attribute__ ((__packed__)) {
    uint8_t chip_id : 8;
    uint8_t zeros : 8;
} reg_chip_id_t;

/* Configuration Registers */
#define REG_SUPPLY_SENSOR_ADC_BUFF_BIAS_CURRENT           0
#define REG_ADC_OUT_FORMAT_DSP_OFFSET_RM_AUX_DIG_OUT      1
#define REG_IMPEDENCE_CHECK_CONTROL                       2
#define REG_IMPEDENCE_CHECK_DAC                           3
#define REG_ONCHIP_AMP_BW_SEL_ONE                         4
#define REG_ONCHIP_AMP_BW_SEL_TWO                         5
#define REG_ONCHIP_AMP_BW_SEL_THREE                       6
#define REG_ONCHIP_AMP_BW_SEL_FOUR                        7
#define REG_IND_AC_AMP_POWER                              8
#define REG_AMP_FAST_SETTLE_TRGD                          10
#define REG_AMP_LOWER_CUTOFF_FREQ_SEL_TRGD                12


#define REG_STIM_ENABLE_A                           32
#define REG_STIM_ENABLE_B                           33
#define REG_STIM_CURRENT_STEP_SIZE                  34

#define REG_STIM_BIAS_VOL                           35
#define REG_CHARGE_RECOVERY_TARGET                  36
#define REG_CHARGE_RECOVERY_LIM                     37
#define REG_IND_DC_AMP_POWER                        38

#define REG_STIM_ON_TRGD                            42
#define REG_STIM_POLARITY_TRGD                      44
#define REG_CHARGE_RECOVERY_SWTICH_TRGD             46
#define REG_CHARGE_RECOVERY_ENABLE_TRGD             48

// These 16 regs correspond to each of the 16 channels
#define REG_NEG_STIM_CURRENT_MAG_TRGD_BASE          64
#define REG_NEG_STIM_CURRENT_MAG_TRGD_END           79

#define REG_POS_STIM_CURRENT_MAG_TRGD_BASE          96
#define REG_POS_STIM_CURRENT_MAG_TRGD_END           111


/*Intan Instruction Set - Read Pg.34 */
#define INTAN_RWC_COMMAND_HEADER_OFFSET            (30) //Read/Write/Convert commands share same header offset, bits are different
#define INTAN_RWC_COMMAND_HEADER_MASK              (0x3 << INTAN_RWC_COMMAND_HEADER_OFFSET) // 2 bit mask, bits 30 and 31


/* -- READ Command -- */
#define INTAN_READ_HEADER            (0b11 << INTAN_RWC_COMMAND_HEADER_OFFSET)
// Setting the U flag to one updates all “triggered registers” to new values that were previously programmed
#define INTAN_READ_U_FLAG_OFFSET     (29)
// Setting the M flag to one clears the compliance monitor register (Register 40).
#define INTAN_READ_M_FLAG_OFFSET     (28)
#define INTAN_READ_REG_OFFSET        (16)
#define INTAN_READ_REG_MASK          (0xFF)
#define INTAN_READ_REG(reg_val)      ((INTAN_READ_REG_MASK & reg_val) << INTAN_READ_REG_OFFSET)

// Call this macro reg value, and u,m = 0 or 1
#define INTAN_READ(reg_val, u, m)    (INTAN_READ_HEADER | INTAN_READ_REG(reg_val) | \
                                             (u << INTAN_READ_U_FLAG_OFFSET) | (m << INTAN_READ_M_FLAG_OFFSET))


/* -- WRITE Command -- */
#define INTAN_WRITE_HEADER            (0b10 << INTAN_RWC_COMMAND_HEADER_OFFSET)
// Setting the U flag to one updates all “triggered registers” to new values that were previously programmed
#define INTAN_WRITE_U_FLAG_OFFSET     (29)
// Setting the M flag to one clears the compliance monitor register (Register 40).
#define INTAN_WRITE_M_FLAG_OFFSET     (28)
#define INTAN_WRITE_REG_OFFSET        (16)
#define INTAN_WRITE_REG_MASK          (0xFF)
#define INTAN_WRITE_DATA_MASK         (0xFFFF)
#define INTAN_WRITE_REG(reg_val)      ((INTAN_WRITE_REG_MASK & reg_val) << INTAN_WRITE_REG_OFFSET)

#define INTAN_WRITE(reg_val, data, u, m)  (INTAN_WRITE_HEADER | INTAN_WRITE_REG(reg_val) | \
                                           (INTAN_WRITE_DATA_MASK & data) | (u << INTAN_WRITE_U_FLAG_OFFSET) | (m << INTAN_WRITE_M_FLAG_OFFSET))



/* -- CLEAR Command -- */
#define INTAN_CLEAR_HEADER             (0b01101010 << 24)
#define INTAN_CLEAR                    INTAN_CLEAR_HEADER


/* -- CONVERT Command -- */
#define INTAN_CONVERT_HEADER            (0b00 << INTAN_RWC_COMMAND_HEADER_OFFSET)
// Setting the U flag to one updates all “triggered registers” to new values that were previously programmed
#define INTAN_CONVERT_U_FLAG_OFFSET     (29)
// Setting the M flag to one clears the compliance monitor register (Register 40).
#define INTAN_CONVERT_M_FLAG_OFFSET     (28)
// If the D (DC amplifier) flag of a CONVERT command is set to one then the DC low-gain amplifier of channel C is also
// sampled (with 10-bit resolution), and its value is returned in the lower 10 bits of the result.
#define INTAN_CONVERT_D_FLAG_OFFSET     (27)
// If the H (High-pass filter) flag of a CONVERT command is set to one when DSP offset removal is enabled (see “DSP HighPass Filter for Offset Removal” section) then 
// the output of the digital high-pass filter associated with amplifier channel C is reset to
// zero. This can be used to rapidly recover from a large transient and settle to baseline
#define INTAN_CONVERT_H_FLAG_OFFSET     (26)
#define INTAN_CONVERT_CHANNEL_OFFSET    (16)
#define INTAN_CONVERT_CHANNEL_MASK      (0x3F)
#define INTAN_CONVERT_CHANNEL(c)        ((c & INTAN_CONVERT_CHANNEL_MASK) << INTAN_CONVERT_CHANNEL_OFFSET)

#define INTAN_CONVERT(c, u, m, d, h)            (INTAN_CONVERT_HEADER | INTAN_CONVERT_CHANNEL(c) | (u << INTAN_CONVERT_U_FLAG_OFFSET) | \
                                                (m << INTAN_CONVERT_M_FLAG_OFFSET) | (d << INTAN_CONVERT_D_FLAG_OFFSET) | \
                                                (h << INTAN_CONVERT_H_FLAG_OFFSET))




int intan_headstage_init(void);
void intan_continuous_sample(void);
void intan_dump_channel_data(void);
void intan_send_and_receive(uint32_t command);
void intan_step_up_stim(void);
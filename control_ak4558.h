/*
 * HiFi Audio Codec Module support library for Teensy 3.x
 *
 * Copyright 2015, Michele Perla
 *
 */
#ifndef control_ak4558_h_
#define control_ak4558_h_

#include "AudioControl.h"

#define AK4558_SERIAL_DEBUG 1
//if 1, then Serial Monitor will show debug information about configuration of the AK4558

// for Teensy audio lib operation the following settings are needed
// 1fs = 44.1 KHz
// sample size = 16 bits
// MCKI : 11.2896 MHz
// BICK : 1.4112 MHz
// LRCK : 44.100 KHz
// to do so we need to set the following bits:
// PMPLL = 0 (EXT Slave Mode; disables internal PLL and uses ext. clock) (by DEFAULT)
// ACKS = 0 (Manual Setting Mode; disables automatic clock selection) (by DEFAULT)
// DFS1-0 = 00 (Sampling Speed = Normal Speed Mode, default)
// MCKS1-0 = 00 (Master Clock Input Frequency Select, set 256fs for Normal Speed Mode -> 11.2896 MHz)
// BCKO1-0 = 00 (BICK Output Frequency at Master Mode = 32fs = 1.4112 MHz)
// TDM1-0 = 00 (Time Division Multiplexing mode OFF) (by DEFAULT)
// DIF2-1-0 = 011 ( 16 bit I2S compatible when BICK = 32fs)

#ifndef PIN_PDN
#define PIN_PDN 1
#endif
// Power-Down & Reset Mode Pin
// “L”: Power-down and Reset, “H”: Normal operation
// The AK4558 should be reset once by bringing PDN pin = “L”

#ifndef AK4558_CAD1
#define AK4558_CAD1 1
#endif
// Chip Address 1 pin
// set to 'H' by default, configurable to 'L' via a jumper on bottom side of the board

#ifndef AK4558_CAD0
#define AK4558_CAD0 1
#endif
// Chip Address 0 pin
// set to 'H' by default, configurable to 'L' via a jumper on bottom side of the board

#define AK4558_I2C_ADDR (0x10 + (AK4558_CAD1<<1) + AK4558_CAD0)
// datasheet page 81:
// This address is 7 bits long followed by the eighth bit that is a data direction bit (R/W). 
// The most significant five bits of the slave address are fixed as “00100”. The next bits are
// CAD1 and CAD0 (device address bit). These bits identify the specific device on the bus.
// The hard-wired input pins (CAD1 and CAD0) set these device address bits (Figure 69)

// Power Management register
#define AK4558_PWR_MNGT 	0x00
// D4		D3		D2		D1		D0
// PMADR 	PMADL 	PMDAR 	PMDAL 	RSTN
#define AK4558_PMADR	(1u<<4)
#define AK4558_PMADL 	(1u<<3)
// PMADL/R: ADC L/Rch Power Management
// 0: ADC L/Rch Power Down (default)
// 1: Normal Operation
#define AK4558_PMDAR	(1u<<2)
#define AK4558_PMDAL	(1u<<1)
// PMDAL/R: DAC L/Rch Power Management
// 0: DAC L/Rch Power Down (default)
// 1: Normal Operation
#define AK4558_RSTN		(1u)
// RSTN: Internal Timing Reset
// 0: Reset Register values are not reset.
// 1: Normal Operation (default)


// PLL Control register
#define AK4558_PLL_CTRL 	0X01
// D4		D3		D2		D1		D0
// PLL3		PLL2	PLL1	PLL0 	PMPLL
#define AK4558_PLL3		(1u<<4)
#define AK4558_PLL2		(1u<<3)
#define AK4558_PLL1		(1u<<2)
#define AK4558_PLL0		(1u<<1)
// PLL3-0: PLL Reference Clock Select (Table 16)
// Default: “0010” (BICK pin=64fs)
#define AK4558_PMPLL	(1u)
// PMPLL: PLL Power Management
// 0: EXT Mode and Power down (default)
// 1: PLL Mode and Power up


// DAC TDM register
#define AK4558_DAC_TDM		0X02
// D1		D0
// SDS1 	SDS0
#define AK4558_SDS1		(1u<<1)
#define AK4558_SDS0		(1u)
// SDS1-0: DAC TDM Data Select (Table 24)
// Default: “00”


// Control 1 register
#define AK4558_CTRL_1		0X03
// D7		D6		D5		D4		D3		D2		D1		D0
// TDM1		TDM0	DIF2	DIF1	DIF0	ATS1	ATS0 	SMUTE
#define AK4558_TDM1		(1u<<7)
#define AK4558_TDM0		(1u<<6)
// TDM1-0: TDM Format Select (Table 23, Table 25, Table 26)
// Default: “00” (Stereo Mode)
#define AK4558_DIF2		(1u<<5)
#define AK4558_DIF1		(1u<<4)
#define AK4558_DIF0		(1u<<3)
// DIF2-0: Audio Interface Format Mode Select (Table 23)
// Default: “111” (32bit I2S)
#define AK4558_ATS1		(1u<<2)
#define AK4558_ATS0		(1u<<1)
// ATS1-0: Transition Time Setting of Digital Attenuator (Table 31)
// Default: “00”
#define AK4558_SMUTE	(1u)
// SMUTE: Soft Mute Enable
// 0: Normal Operation (default)
// 1: All DAC outputs are soft muted.


// Control 2 register
#define AK4558_CTRL_2		0X04
// D4		D3		D2		D1		D0
// MCKS1	MCKS0	DFS1	DFS0	ACKS
#define AK4558_MCKS1	(1u<<4)
#define AK4558_MCKS0	(1u<<3)
// MCKS1-0: Master Clock Input Frequency Select (Table 9, follows):
// 	MCKS1 	MCKS0	NSM		DSM		QSM
// 	0		0		256fs	256fs	128fs
// 	0		1		384fs	256fs	128fs
// 	1		0		512fs	256fs	128fs (default)
// 	1		1		768fs	256fs	128fs
#define AK4558_DFS1		(1u<<2)
#define AK4558_DFS0		(1u<<1)
// DFS1-0: Sampling Speed Control (Table 8)
// The setting of DFS1-0 bits is ignored when ACKS bit =“1”.
#define AK4558_ACKS		(1u)
// ACKS: Automatic Clock Recognition Mode
// 0: Disable, Manual Setting Mode (default)
// 1: Enable, Auto Setting Mode
// When ACKS bit = “1”, master clock frequency is detected automatically. In this case, the setting of
// DFS1-0 bits is ignored. When ACKS bit = “0”, DFS1-0 bits set the sampling speed mode. The MCKI
// frequency of each mode is detected automatically.


// Mode Control register
#define AK4558_MODE_CTRL	0X05
// D6		D5		D4 		D3 		D2 		D1 		D0
// FS3 		FS2 	FS1 	FS0 	BCKO1 	BCKO0 	LOPS
#define AK4558_FS3		(1u<<6)
#define AK4558_FS2		(1u<<5)
#define AK4558_FS1		(1u<<4)
#define AK4558_FS0		(1u<<3)
// FS3-0: Sampling Frequency (Table 17, Table 18)
// Default: “0101”
#define AK4558_BCKO1	(1u<<2)
#define AK4558_BCKO0	(1u<<1)
// BCKO1-0: BICK Output Frequency Setting in Master Mode (Table 21)
// Default: “01” (64fs)
#define AK4558_LOPS		(1u<<0)
// LOPS: Power-save Mode of LOUT/ROUT
// 0: Normal Operation (default)
// 1: Power-save Mode


// Filter Setting register
#define AK4558_FLTR_SET 	0x06
// D7		D6		D5		D4		D3		D2		D1		D0
// FIRDA2 	FIRDA1 	FIRDA0	SLDA 	SDDA 	SSLOW 	DEM1 	DEM0
#define AK4558_FIRDA2	(1u<<7)
#define AK4558_FIRDA1	(1u<<6)
#define AK4558_FIRDA0	(1u<<5)
// FIRDA2-0: Out band noise eliminating Filters Setting (Table 32)
// default: “001” (48kHz)
#define AK4558_SLDA		(1u<<4)
// SLDA: DAC Slow Roll-off Filter Enable (Table 28)
// 0: Sharp Roll-off filter (default)
// 1: Slow Roll-off Filter
#define AK4558_SDDA		(1u<<3)
// SDDA: DAC Short delay Filter Enable (Table 28)
// 0: Normal filter
// 1: Short delay Filter (default)
#define AK4558_SSLOW	(1u<<2)
// SSLOW: Digital Filter Bypass Mode Enable
// 0: Roll-off filter (default)
// 1: Super Slow Roll-off Mode
#define AK4558_DEM1		(1u<<1)
#define AK4558_DEM0		(1u)
// DEM1-0: De-emphasis response control for DAC (Table 22)
// Default: “01”, OFF


// HPF Enable, Filter Setting
#define AK4558_HPF_EN_FLTR_SET 0x07
// D3		D2		D1		D0
// SLAD 	SDAD 	HPFER 	HPFEL
#define AK4558_SLAD		(1u<<3)
// SLAD: ADC Slow Roll-off Filter Enable (Table 27)
// 0: Sharp Roll-off filter (default)
// 1: Slow Roll-off Filter
#define AK4558_SDAD		(1u<<2)
// SDAD: ADC Short delay Filter Enable (Table 27)
// 0: Normal filter
// 1: Short delay Filter (default)
#define AK4558_HPFER	(1u<<1)
#define AK4558_HPFEL	(1u)
// HPFEL/R: ADC HPF L/Rch Setting
// 0: HPF L/Rch OFF
// 1: HPF L/Rch ON (default)


// LOUT Volume Control register
#define AK4558_LOUT_VOL		0X08
// D7		D6		D5		D4		D3		D2		D1		D0
// ATL7		ATL6	ATL5	ATL4	ATL3	ATL2 	ATL1 	ATL0
//
// ATL 7-0: Attenuation Level (Table 30)
// Default:FF(0dB)


// ROUT Volume Control register
#define AK4558_ROUT_VOL		0X09
// D7		D6		D5		D4		D3		D2		D1		D0
// ATR7		ATR6	ATR5 	ATR4	ATR3	ATR2	ATR1	ATR0
//
// ATR 7-0: Attenuation Level (Table 30)
// Default:FF(0dB)

class AudioControlAK4558 : public AudioControl
{
public:
	bool enable(void);		//enables the CODEC, does not power up ADC nor DAC (use enableIn() and enableOut() for selective power up)
	bool enableIn(void);	//powers up ADC
	bool enableOut(void);	//powers up DAC
	bool disable(void) { return (disableIn()&&disableOut()); }	//powers down ADC/DAC
	bool disableIn(void);	//powers down ADC
	bool disableOut(void);	//powers down DAC
	bool volume(float n);	//sets LOUT/ROUT volume to n (range 0.0 - 1.0)
	bool volumeLeft(float n);	//sets LOUT volume to n (range 0.0 - 1.0)
	bool volumeRight(float n);	//sets ROUT volume to n (range 0.0 - 1.0)
	bool inputLevel(float n) { return false; }	//not supported by AK4558
	bool inputSelect(int n) { return false; }	//sets inputs to mono left, mono right, stereo (default stereo), not yet implemented
private:
	uint8_t registers[10];
	void initConfig(void);
	void readConfig(void);
	bool write(unsigned int reg, unsigned int val);
	uint8_t convertVolume(float vol);
};

#endif
/*
 * HiFi Audio Codec Module support library for Teensy 3.x
 *
 * Copyright 2015, Michele Perla
 *
 */
 
#include "control_ak4558.h"
#include "Wire.h"

#if defined(ARDUINO_ARCH_SAMD)
#include <Arduino.h>
#endif

void AudioControlAK4558::initConfig(void)
{
	// puts all default registers values inside an array
	// this allows us to modify registers locally using annotation like follows:
	//
	// registers[AK4558_CTRL_1] &= ~AK4558_DIF2;
	// registers[AK4558_CTRL_1] |= AK4558_DIF1 | AK4558_DIF0;
	//
	// after manipulation, we can write the entire register value on the CODEC
	uint8_t n = 0;
	Wire.requestFrom(AK4558_I2C_ADDR,10);
	while(Wire.available()) {
#if AK4558_SERIAL_DEBUG > 0
		Serial.print("Register ");
		Serial.print(n);
		Serial.print(" = ");
#endif
		registers[n++] = Wire.read();
#if AK4558_SERIAL_DEBUG > 0
		Serial.println(registers[n-1], BIN);
#endif
	}
}

void AudioControlAK4558::readConfig(void)
{
	// reads registers values
	uint8_t n = 0;
	uint8_t c = 0;
	Wire.requestFrom(AK4558_I2C_ADDR, 10);
	while(Wire.available()) {
		Serial.print("Register ");
		Serial.print(n++);
		Serial.print(" = ");
		c = Wire.read();
		Serial.println(c, BIN);
	}
}

bool AudioControlAK4558::write(unsigned int reg, unsigned int val)
{
	Wire.beginTransmission(AK4558_I2C_ADDR);
	Wire.write(reg);
	Wire.write(val);
	return (Wire.endTransmission(true)==0);
}

bool AudioControlAK4558::enableIn(void)
{
	// ADC setup (datasheet page 74
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable ADC");
#endif

	// ignore this, leaving default values - ADC: Set up the de-emphasis filter (Addr = 07H).
	
	registers[AK4558_PWR_MNGT] |= AK4558_PMADR | AK4558_PMADL;
	write(AK4558_PWR_MNGT, registers[AK4558_PWR_MNGT]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: PWR_MNGT set to ");
	Serial.println(registers[AK4558_PWR_MNGT], BIN);
#endif
	delay(300);
	// Power up the ADC: PMADL = PMADR bits = “0” → “1”
	// Initialization cycle of the ADC is 5200/fs @Normal mode. The SDTO pin outputs “L” during initialization.
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable ADC - Done");
#endif
	return true;
}
bool AudioControlAK4558::enableOut(void)
{
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable DAC");
#endif
	// DAC Output setup (datasheet page 75)

	registers[AK4558_MODE_CTRL] |= AK4558_LOPS;
	write(AK4558_MODE_CTRL, registers[AK4558_MODE_CTRL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: MODE_CTRL set to ");
	Serial.println(registers[AK4558_MODE_CTRL], BIN);
#endif
	// Set the DAC output to power-save mode: LOPS bit “0” → “1”
	
	// ignore this, leaving default values - DAC: Set up the digital filter mode.
	// ignore this, leaving default values - Set up the digital output volume (Address = 08H, 09H).
	
	registers[AK4558_PWR_MNGT] |= AK4558_PMDAR | AK4558_PMDAL;
	write(AK4558_PWR_MNGT, registers[AK4558_PWR_MNGT]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: PWR_MNGT set to ");
	Serial.println(registers[AK4558_PWR_MNGT], BIN);
#endif
	delay(300);
	// Power up the DAC: PMDAL = PMDAR bits = “0” → “1”
	// Outputs of the LOUT and ROUT pins start rising. Rise time is 300ms (max.) when C = 1μF.
	
	registers[AK4558_MODE_CTRL] &= ~AK4558_LOPS;
	write(AK4558_MODE_CTRL, registers[AK4558_MODE_CTRL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: MODE_CTRL set to ");
	Serial.println(registers[AK4558_MODE_CTRL], BIN);
#endif
	// Release power-save mode of the DAC output: LOPS bit = “1” → “0”
	// Set LOPS bit to “0” after the LOUT and ROUT pins output “H”. Sound data will be output from the
	// LOUT and ROUT pins after this setting.
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable DAC - Done");
#endif
	return true;
}

bool AudioControlAK4558::enable(void)
{
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable device");
#endif
	// Power Up and Reset
	// Clock Setup (datasheet page 72)
	pinMode(PIN_PDN, OUTPUT);
	digitalWrite(0, LOW);
	delay(1);
	digitalWrite(0, HIGH);
	// After Power Up: PDN pin “L” → “H”
	// “L” time of 150ns or more is needed to reset the AK4558.
	delay(20);
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: PDN is HIGH (device reset)");
#endif
	// Control register settings become available in 10ms (min.) when LDOE pin = “H”
	Wire.begin();
	initConfig();
	// access all registers to store locally their default values
	
	// DIF2-0, DFS1-0 and ACKS bits must be set before MCKI, LRCK and BICK are supplied
	
	// PMPLL = 0 (EXT Slave Mode; disables internal PLL and uses ext. clock) 	(by DEFAULT)
	// ACKS = 0 (Manual Setting Mode; disables automatic clock selection) 		(by DEFAULT)
	// DFS1-0 = 00 (Sampling Speed = Normal Speed Mode)							(by DEFAULT)
	// TDM1-0 = 00 (Time Division Multiplexing mode OFF) 						(by DEFAULT)
	
	registers[AK4558_CTRL_1] &= ~AK4558_DIF2;
	registers[AK4558_CTRL_1] |= AK4558_DIF1 | AK4558_DIF0;
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: CTRL_1 set to ");
	Serial.println(registers[AK4558_CTRL_1], BIN);
#endif
	// DIF2-1-0 = 011 ( 16 bit I2S compatible when BICK = 32fs)
	registers[AK4558_CTRL_2] &= ~AK4558_MCKS1;
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: CTRL_2 set to ");
	Serial.println(registers[AK4558_CTRL_2], BIN);
#endif
	// MCKS1-0 = 00 (Master Clock Input Frequency Select, set 256fs for Normal Speed Mode -> 11.2896 MHz)
	registers[AK4558_MODE_CTRL] &= ~AK4558_BCKO0;
	// BCKO1-0 = 00 (BICK Output Frequency at Master Mode = 32fs = 1.4112 MHz)
	registers[AK4558_MODE_CTRL] |= AK4558_FS1;
	// Set up the sampling frequency (FS3-0 bits). The ADC must be powered-up in consideration of PLL
	// lock time. (in this case (ref. table 17): Set clock to mode 5 / 44.100 KHz)
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: MODE_CTRL set to ");
	Serial.println(registers[AK4558_MODE_CTRL], BIN);
#endif
	// BCKO1-0 = 00 (BICK Output Frequency at Master Mode = 32fs = 1.4112 MHz)
	Wire.beginTransmission(AK4558_I2C_ADDR);
	Wire.write(AK4558_CTRL_1);
	Wire.write(registers[AK4558_CTRL_1]);
	Wire.write(registers[AK4558_CTRL_2]);
	Wire.write(registers[AK4558_MODE_CTRL]);
	Wire.endTransmission();
	// Write configuration registers in a single write operation (datasheet page 81):
	// The AK4558 can perform more than one byte write operation per sequence. After receipt of the third byte
	// the AK4558 generates an acknowledge and awaits the next data. The master can transmit more than
	// one byte instead of terminating the write cycle after the first data byte is transferred. After receiving each
	// data packet the internal address counter is incremented by one, and the next data is automatically taken
	// into the next address.
	
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable device - Done");
#endif
	return true;
}

bool AudioControlAK4558::disableIn(void)
{
	// ADC power-down (datasheet page 74
	
	registers[AK4558_PWR_MNGT] &= ~AK4558_PMADR | ~AK4558_PMADL;
	write(AK4558_PWR_MNGT, registers[AK4558_PWR_MNGT]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: PWR_MNGT set to ");
	Serial.println(registers[AK4558_PWR_MNGT], BIN);
#endif
	// Power down ADC: PMADL = PMADR bits = “1” → “0”
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Enable ADC - Done");
#endif
	return true;
}

bool AudioControlAK4558::disableOut(void)
{
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Disable DAC");
#endif
	// DAC Output power-down (datasheet page 75)

	registers[AK4558_MODE_CTRL] |= AK4558_LOPS;
	write(AK4558_MODE_CTRL, registers[AK4558_MODE_CTRL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: MODE_CTRL set to ");
	Serial.println(registers[AK4558_MODE_CTRL], BIN);
#endif
	// Set the DAC output to power-save mode: LOPS bit “0” → “1”
	
	registers[AK4558_PWR_MNGT] &= ~AK4558_PMDAR | ~AK4558_PMDAL;
	write(AK4558_PWR_MNGT, registers[AK4558_PWR_MNGT]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: PWR_MNGT set to ");
	Serial.println(registers[AK4558_PWR_MNGT], BIN);
#endif
	delay(300);
	// Power down the DAC: PMDAL = PMDAR bits = “1” → “0”
	// Outputs of the LOUT and ROUT pins start falling. Rise time is 300ms (max.) when C = 1μF.
	
	registers[AK4558_MODE_CTRL] &= ~AK4558_LOPS;
	write(AK4558_MODE_CTRL, registers[AK4558_MODE_CTRL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: MODE_CTRL set to ");
	Serial.println(registers[AK4558_MODE_CTRL], BIN);
#endif
	// Release power-save mode of the DAC output: LOPS bit = “1” → “0”
	// Set LOPS bit to “0” after outputs of the LOUT and ROUT pins fall to “L”.
#if AK4558_SERIAL_DEBUG > 0
	Serial.println("AK4558: Disable DAC - Done");
#endif
	return true;
}

uint8_t AudioControlAK4558::convertVolume(float vol)
{
	// Convert float (range 0.0-1.0) to unsigned char (range 0x00-0xFF)
	uint8_t temp = ((uint32_t)vol)>>22;
	return temp;
}

bool AudioControlAK4558::volume(float n)
{
	// Set DAC output volume
	uint8_t vol = convertVolume(n);
	registers[AK4558_LOUT_VOL] = vol;
	registers[AK4558_ROUT_VOL] = vol;
	Wire.beginTransmission(AK4558_I2C_ADDR);
	Wire.write(AK4558_LOUT_VOL);
	Wire.write(registers[AK4558_LOUT_VOL]);
	Wire.write(registers[AK4558_ROUT_VOL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: LOUT_VOL set to ");
	Serial.println(registers[AK4558_LOUT_VOL], BIN);
	Serial.print("AK4558: ROUT_VOL set to ");
	Serial.println(registers[AK4558_ROUT_VOL], BIN);
#endif
	return (Wire.endTransmission(true)==0);
}

bool AudioControlAK4558::volumeLeft(float n)
{
	// Set DAC left output volume
	uint8_t vol = convertVolume(n);
	registers[AK4558_LOUT_VOL] = vol;
	bool ret = write(AK4558_LOUT_VOL, registers[AK4558_LOUT_VOL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: LOUT_VOL set to ");
	Serial.println(registers[AK4558_LOUT_VOL], BIN);
#endif
	return ret;
}

bool AudioControlAK4558::volumeRight(float n)
{
	// Set DAC right output volume
	uint8_t vol = convertVolume(n);
	registers[AK4558_ROUT_VOL] = vol;
	bool ret = write(AK4558_ROUT_VOL, registers[AK4558_ROUT_VOL]);
#if AK4558_SERIAL_DEBUG > 0
	Serial.print("AK4558: ROUT_VOL set to ");
	Serial.println(registers[AK4558_ROUT_VOL], BIN);
#endif
	return ret;
}
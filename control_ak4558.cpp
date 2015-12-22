/*
 * HiFi Audio Codec Module support library for Teensy 3.x
 *
 * Copyright 2015, Michele Perla
 *
 */
 
#include "control_ak4558.h"
#include "Wire.h"

void AudioControlAK4558::readInitConfig(void)
{
	// puts all default registers values inside an array
	// this allows us to modify registers locally using annotation like follows:
	//
	// registers[AK4558_CTRL_1] &= ~AK4558_DIF2;
	// registers[AK4558_CTRL_1] |= AK4558_DIF1 | AK4558_DIF0;
	//
	// after manipulation, we can write the entire register value on the CODEC
	unsigned int n = 0;
	Wire.requestFrom(AK4558_I2C_ADDR, 10);
	while(Wire.available()) {
		Serial.print("Register ");
		Serial.print(n);
		Serial.print(" = ");
		registers[n++] = Wire.read();
		Serial.println(registers[n-1]);
	}
}

bool AudioControlAK4558::write(unsigned int reg, unsigned int val)
{
	Wire.beginTransmission(AK4558_I2C_ADDR);
	Wire.write(reg);
	Wire.write(val);
	return (Wire.endTransmission()==0);
}

bool AudioControlAK4558::enable(void)
{
	// Power Up and Reset
	// Clock Setup (datasheet page 72)
	digitalWrite(PIN_PDN, LOW);
	delay(1);
	digitalWrite(PIN_PDN, HIGH);
	// After Power Up: PDN pin “L” → “H”
	// “L” time of 150ns or more is needed to reset the AK4558.
	delay(20);
	Serial.println("PDN is HIGH");
	// Control register settings become available in 10ms (min.) when LDOE pin = “H”
	Wire.begin();
	readInitConfig();
	// access all registers to store locally their default values
	
	// DIF2-0, DFS1-0 and ACKS bits must be set before MCKI, LRCK and BICK are supplied
	
	// PMPLL = 0 (EXT Slave Mode; disables internal PLL and uses ext. clock) 	(by DEFAULT)
	// ACKS = 0 (Manual Setting Mode; disables automatic clock selection) 		(by DEFAULT)
	// DFS1-0 = 00 (Sampling Speed = Normal Speed Mode)							(by DEFAULT)
	// TDM1-0 = 00 (Time Division Multiplexing mode OFF) 						(by DEFAULT)
	
	registers[AK4558_CTRL_1] &= ~AK4558_DIF2;
	registers[AK4558_CTRL_1] |= AK4558_DIF1 | AK4558_DIF0;
	// DIF2-1-0 = 011 ( 16 bit I2S compatible when BICK = 32fs)
	registers[AK4558_CTRL_2] &= ~AK4558_MCKS1;
	// MCKS1-0 = 00 (Master Clock Input Frequency Select, set 256fs for Normal Speed Mode -> 11.2896 MHz)
	registers[AK4558_MODE_CTRL] &= ~AK4558_BCKO0;
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
	
	// ADC/DAC Output setup (datasheet pages 74, 75)
	registers[AK4558_PLL_CTRL] |= AK4558_PLL2;
	registers[AK4558_PLL_CTRL] &= ~AK4558_PLL1;
	write(AK4558_I2C_ADDR, registers[AK4558_PLL_CTRL]);
	delay(10);
	// as per table 16, set PLL_CTRL.PLL3-2-1-0 to 0101 for MICK as PLL Reference, 11.2896 MHz
	// also, wait 10 ms for PLL lock
	// TODO: IS IT NEEDED?
	// Set the DAC output to power-save mode: LOPS bit “0” → “1”
	registers[AK4558_MODE_CTRL] |= AK4558_FS1 | AK4558_LOPS;
	write(AK4558_I2C_ADDR, registers[AK4558_MODE_CTRL]);
	// Set up the sampling frequency (FS3-0 bits). The ADC must be powered-up in consideration of PLL
	// lock time. (in this case (ref. table 17): Set clock to mode 5 / 44.100 KHz)
	// Set up the audio format (Addr=03H). (in this case: TDM1-0 = 00 (Time Division Multiplexing mode OFF) by default)
	
	// ignore this, leaving default values - ADC: Set up the de-emphasis filter (Addr = 07H).
	// ignore this, leaving default values - DAC: Set up the digital filter mode.
	// ignore this, leaving default values - Set up the digital output volume (Address = 08H, 09H).
	
	registers[AK4558_PWR_MNGT] |= AK4558_PMADR | AK4558_PMADL | AK4558_PMDAR | AK4558_PMDAL;
	delay(300);
	// Power up the ADC: PMADL = PMADR bits = “0” → “1”
	// Initialization cycle of the ADC is 5200/fs @Normal mode. The SDTO pin outputs “L” during initialization.
	// Power up the DAC: PMDAL = PMDAR bits = “0” → “1”
	// Outputs of the LOUT and ROUT pins start rising. Rise time is 300ms (max.) when C = 1μF.
	
	registers[AK4558_MODE_CTRL] &= ~AK4558_LOPS;
	write(AK4558_I2C_ADDR, registers[AK4558_MODE_CTRL]);
	// Release power-save mode of the DAC output: LOPS bit = “1” → “0”
	// Set LOPS bit to “0” after the LOUT and ROUT pins output “H”. Sound data will be output from the
	// LOUT and ROUT pins after this setting.
	Serial.println("Setup ended");
	return true;
}

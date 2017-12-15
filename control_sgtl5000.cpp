/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "control_sgtl5000.h"
#include "Wire.h"

#if defined(ARDUINO_ARCH_SAMD)
#include <Arduino.h>
#endif

#define CHIP_ID				0x0000
// 15:8 PARTID		0xA0 - 8 bit identifier for SGTL5000
// 7:0  REVID		0x00 - revision number for SGTL5000.

#define CHIP_DIG_POWER			0x0002
// 6	ADC_POWERUP	1=Enable, 0=disable the ADC block, both digital & analog, 
// 5	DAC_POWERUP	1=Enable, 0=disable the DAC block, both analog and digital
// 4	DAP_POWERUP	1=Enable, 0=disable the DAP block
// 1	I2S_OUT_POWERUP	1=Enable, 0=disable the I2S data output
// 0	I2S_IN_POWERUP	1=Enable, 0=disable the I2S data input

#define CHIP_CLK_CTRL			0x0004
// 5:4	RATE_MODE	Sets the sample rate mode. MCLK_FREQ is still specified
//			relative to the rate in SYS_FS
//				0x0 = SYS_FS specifies the rate
//				0x1 = Rate is 1/2 of the SYS_FS rate
//				0x2 = Rate is 1/4 of the SYS_FS rate
//				0x3 = Rate is 1/6 of the SYS_FS rate
// 3:2	SYS_FS		Sets the internal system sample rate (default=2)
//				0x0 = 32 kHz
//				0x1 = 44.1 kHz
//				0x2 = 48 kHz
//				0x3 = 96 kHz
// 1:0	MCLK_FREQ	Identifies incoming SYS_MCLK frequency and if the PLL should be used
//				0x0 = 256*Fs
//				0x1 = 384*Fs
//				0x2 = 512*Fs
//				0x3 = Use PLL
//				The 0x3 (Use PLL) setting must be used if the SYS_MCLK is not
//				a standard multiple of Fs (256, 384, or 512). This setting can
//				also be used if SYS_MCLK is a standard multiple of Fs.
//				Before this field is set to 0x3 (Use PLL), the PLL must be
//				powered up by setting CHIP_ANA_POWER->PLL_POWERUP and
//				CHIP_ANA_POWER->VCOAMP_POWERUP.  Also, the PLL dividers must
//				be calculated based on the external MCLK rate and
//				CHIP_PLL_CTRL register must be set (see CHIP_PLL_CTRL register
//				description details on how to calculate the divisors).

#define CHIP_I2S_CTRL			0x0006
// 8	SCLKFREQ	Sets frequency of I2S_SCLK when in master mode (MS=1). When in slave
//			mode (MS=0), this field must be set appropriately to match SCLK input
//			rate.
//				0x0 = 64Fs
//				0x1 = 32Fs - Not supported for RJ mode (I2S_MODE = 1)
// 7	MS		Configures master or slave of I2S_LRCLK and I2S_SCLK.
//				0x0 = Slave: I2S_LRCLK an I2S_SCLK are inputs
//				0x1 = Master: I2S_LRCLK and I2S_SCLK are outputs
//				NOTE: If the PLL is used (CHIP_CLK_CTRL->MCLK_FREQ==0x3),
//				the SGTL5000 must be a master of the I2S port (MS==1)
// 6	SCLK_INV	Sets the edge that data (input and output) is clocked in on for I2S_SCLK
//				0x0 = data is valid on rising edge of I2S_SCLK
//				0x1 = data is valid on falling edge of I2S_SCLK
// 5:4	DLEN		I2S data length (default=1)
//				0x0 = 32 bits (only valid when SCLKFREQ=0),
//					not valid for Right Justified Mode
//				0x1 = 24 bits (only valid when SCLKFREQ=0)
//				0x2 = 20 bits
//				0x3 = 16 bits
// 3:2	I2S_MODE	Sets the mode for the I2S port
//				0x0 = I2S mode or Left Justified (Use LRALIGN to select)
//				0x1 = Right Justified Mode
//				0x2 = PCM Format A/B
//				0x3 = RESERVED
// 1	LRALIGN		I2S_LRCLK Alignment to data word. Not used for Right Justified mode
//				0x0 = Data word starts 1 I2S_SCLK delay after I2S_LRCLK
//					transition (I2S format, PCM format A)
//				0x1 = Data word starts after I2S_LRCLK transition (left
//					justified format, PCM format B)
// 0	LRPOL		I2S_LRCLK Polarity when data is presented.
//				0x0 = I2S_LRCLK = 0 - Left, 1 - Right
//				1x0 = I2S_LRCLK = 0 - Right, 1 - Left
//				The left subframe should be presented first regardless of
//				the setting of LRPOL.

#define CHIP_SSS_CTRL			0x000A
// 14	DAP_MIX_LRSWAP	DAP Mixer Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAP MIXER Input are swapped.
// 13	DAP_LRSWAP	DAP Mixer Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAP Input are swapped
// 12	DAC_LRSWAP	DAC Input Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the DAC are swapped
// 10	I2S_LRSWAP	I2S_DOUT Swap
//				0x0 = Normal Operation
//				0x1 = Left and Right channels for the I2S_DOUT are swapped
// 9:8	DAP_MIX_SELECT	Select data source for DAP mixer
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = Reserved
// 7:6	DAP_SELECT	Select data source for DAP
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = Reserved
// 5:4	DAC_SELECT	Select data source for DAC (default=1)
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = DAP
// 1:0	I2S_SELECT	Select data source for I2S_DOUT
//				0x0 = ADC
//				0x1 = I2S_IN
//				0x2 = Reserved
//				0x3 = DAP

#define CHIP_ADCDAC_CTRL		0x000E
// 13	VOL_BUSY_DAC_RIGHT Volume Busy DAC Right
//				0x0 = Ready
//				0x1 = Busy - This indicates the channel has not reached its
//					programmed volume/mute level
// 12	VOL_BUSY_DAC_LEFT  Volume Busy DAC Left
//				0x0 = Ready
//				0x1 = Busy - This indicates the channel has not reached its
//					programmed volume/mute level
// 9	VOL_RAMP_EN	Volume Ramp Enable (default=1)
//				0x0 = Disables volume ramp. New volume settings take immediate
//					effect without a ramp
//				0x1 = Enables volume ramp
//				This field affects DAC_VOL. The volume ramp effects both
//				volume settings and mute When set to 1 a soft mute is enabled.
// 8	VOL_EXPO_RAMP	Exponential Volume Ramp Enable
//				0x0 = Linear ramp over top 4 volume octaves
//				0x1 = Exponential ramp over full volume range
//				This bit only takes effect if VOL_RAMP_EN is 1.
// 3	DAC_MUTE_RIGHT	DAC Right Mute (default=1)
//				0x0 = Unmute
//				0x1 = Muted
//				If VOL_RAMP_EN = 1, this is a soft mute.
// 2	DAC_MUTE_LEFT	DAC Left Mute (default=1)
//				0x0 = Unmute
//				0x1 = Muted
//				If VOL_RAMP_EN = 1, this is a soft mute.
// 1	ADC_HPF_FREEZE	ADC High Pass Filter Freeze
//				0x0 = Normal operation
//				0x1 = Freeze the ADC high-pass filter offset register.  The
//				offset continues to be subtracted from the ADC data stream.
// 0	ADC_HPF_BYPASS	ADC High Pass Filter Bypass
//				0x0 = Normal operation
//				0x1 = Bypassed and offset not updated

#define CHIP_DAC_VOL			0x0010
// 15:8	DAC_VOL_RIGHT	DAC Right Channel Volume.  Set the Right channel DAC volume
//			with 0.5017 dB steps from 0 to -90 dB
//				0x3B and less = Reserved
//				0x3C = 0 dB
//				0x3D = -0.5 dB
//				0xF0 = -90 dB
//				0xFC and greater = Muted
//				If VOL_RAMP_EN = 1, there is an automatic ramp to the
//				new volume setting.
// 7:0	DAC_VOL_LEFT	DAC Left Channel Volume.  Set the Left channel DAC volume
//			with 0.5017 dB steps from 0 to -90 dB
//				0x3B and less = Reserved
//				0x3C = 0 dB
//				0x3D = -0.5 dB
//				0xF0 = -90 dB
//				0xFC and greater = Muted
//				If VOL_RAMP_EN = 1, there is an automatic ramp to the
//				new volume setting.

#define CHIP_PAD_STRENGTH		0x0014
// 9:8	I2S_LRCLK	I2S LRCLK Pad Drive Strength (default=1)
//				Sets drive strength for output pads per the table below.
//				 VDDIO    1.8 V     2.5 V     3.3 V
//				0x0 = Disable
//				0x1 =     1.66 mA   2.87 mA   4.02 mA
//				0x2 =     3.33 mA   5.74 mA   8.03 mA
//				0x3 =     4.99 mA   8.61 mA   12.05 mA
// 7:6	I2S_SCLK	I2S SCLK Pad Drive Strength (default=1)
// 5:4	I2S_DOUT	I2S DOUT Pad Drive Strength (default=1)
// 3:2	CTRL_DATA	I2C DATA Pad Drive Strength (default=3)
// 1:0	CTRL_CLK	I2C CLK Pad Drive Strength (default=3)
//				(all use same table as I2S_LRCLK)

#define CHIP_ANA_ADC_CTRL		0x0020
// 8	ADC_VOL_M6DB	ADC Volume Range Reduction
//				This bit shifts both right and left analog ADC volume
//				range down by 6.0 dB.
//				0x0 = No change in ADC range
//				0x1 = ADC range reduced by 6.0 dB
// 7:4	ADC_VOL_RIGHT	ADC Right Channel Volume
//				Right channel analog ADC volume control in 1.5 dB steps.
//				0x0 = 0 dB
//				0x1 = +1.5 dB
//				...
//				0xF = +22.5 dB
//				This range is -6.0 dB to +16.5 dB if ADC_VOL_M6DB is set to 1.
// 3:0	ADC_VOL_LEFT	ADC Left Channel Volume
//				(same scale as ADC_VOL_RIGHT)

#define CHIP_ANA_HP_CTRL		0x0022
// 14:8	HP_VOL_RIGHT	Headphone Right Channel Volume  (default 0x18)
//				Right channel headphone volume control with 0.5 dB steps.
//				0x00 = +12 dB
//				0x01 = +11.5 dB
//				0x18 = 0 dB
//				...
//				0x7F = -51.5 dB
// 6:0	HP_VOL_LEFT	Headphone Left Channel Volume  (default 0x18)
//				(same scale as HP_VOL_RIGHT)

#define CHIP_ANA_CTRL			0x0024
// 8	MUTE_LO		LINEOUT Mute, 0 = Unmute, 1 = Mute  (default 1)
// 6	SELECT_HP	Select the headphone input, 0 = DAC, 1 = LINEIN
// 5	EN_ZCD_HP	Enable the headphone zero cross detector (ZCD)
//				0x0 = HP ZCD disabled
//				0x1 = HP ZCD enabled
// 4	MUTE_HP		Mute the headphone outputs, 0 = Unmute, 1 = Mute (default)
// 2	SELECT_ADC	Select the ADC input, 0 = Microphone, 1 = LINEIN
// 1	EN_ZCD_ADC	Enable the ADC analog zero cross detector (ZCD)
//				0x0 = ADC ZCD disabled
//				0x1 = ADC ZCD enabled
// 0	MUTE_ADC	Mute the ADC analog volume, 0 = Unmute, 1 = Mute (default)

#define CHIP_LINREG_CTRL		0x0026
// 6	VDDC_MAN_ASSN	Determines chargepump source when VDDC_ASSN_OVRD is set.
//				0x0 = VDDA
//				0x1 = VDDIO
// 5	VDDC_ASSN_OVRD	Charge pump Source Assignment Override
//				0x0 = Charge pump source is automatically assigned based
//					on higher of VDDA and VDDIO
//				0x1 = the source of charge pump is manually assigned by
//					VDDC_MAN_ASSN If VDDIO and VDDA are both the same
//					and greater than 3.1 V, VDDC_ASSN_OVRD and
//					VDDC_MAN_ASSN should be used to manually assign
//					VDDIO as the source for charge pump.
// 3:0	D_PROGRAMMING	Sets the VDDD linear regulator output voltage in 50 mV steps.
//			Must clear the LINREG_SIMPLE_POWERUP and STARTUP_POWERUP bits
//			in the 0x0030 (CHIP_ANA_POWER) register after power-up, for
//			this setting to produce the proper VDDD voltage.
//				0x0 = 1.60
//				0xF = 0.85

#define CHIP_REF_CTRL			0x0028 // bandgap reference bias voltage and currents
// 8:4	VAG_VAL		Analog Ground Voltage Control
//				These bits control the analog ground voltage in 25 mV steps.
//				This should usually be set to VDDA/2 or lower for best
//				performance (maximum output swing at minimum THD). This VAG
//				reference is also used for the DAC and ADC voltage reference.
//				So changing this voltage scales the output swing of the DAC
//				and the output signal of the ADC.
//				0x00 = 0.800 V
//				0x1F = 1.575 V
// 3:1	BIAS_CTRL	Bias control
//				These bits adjust the bias currents for all of the analog
//				blocks. By lowering the bias current a lower quiescent power
//				is achieved. It should be noted that this mode can affect
//				performance by 3-4 dB.
//				0x0 = Nominal
//				0x1-0x3=+12.5%
//				0x4=-12.5%
//				0x5=-25%
//				0x6=-37.5%
//				0x7=-50%
// 0	SMALL_POP	VAG Ramp Control
//				Setting this bit slows down the VAG ramp from ~200 to ~400 ms
//				to reduce the startup pop, but increases the turn on/off time.
//				0x0 = Normal VAG ramp
//				0x1 = Slow down VAG ramp

#define CHIP_MIC_CTRL			0x002A // microphone gain & internal microphone bias
// 9:8	BIAS_RESISTOR	MIC Bias Output Impedance Adjustment
//				Controls an adjustable output impedance for the microphone bias.
//				If this is set to zero the micbias block is powered off and
//				the output is highZ.
//				0x0 = Powered off
//				0x1 = 2.0 kohm
//				0x2 = 4.0 kohm
//				0x3 = 8.0 kohm
// 6:4	BIAS_VOLT	MIC Bias Voltage Adjustment
//				Controls an adjustable bias voltage for the microphone bias
//				amp in 250 mV steps. This bias voltage setting should be no
//				more than VDDA-200 mV for adequate power supply rejection.
//				0x0 = 1.25 V
//				...
//				0x7 = 3.00 V
// 1:0	GAIN		MIC Amplifier Gain
//				Sets the microphone amplifier gain. At 0 dB setting the THD
//				can be slightly higher than other paths- typically around
//				~65 dB. At other gain settings the THD are better.
//				0x0 = 0 dB
//				0x1 = +20 dB
//				0x2 = +30 dB
//				0x3 = +40 dB

#define CHIP_LINE_OUT_CTRL		0x002C
// 11:8	OUT_CURRENT	Controls the output bias current for the LINEOUT amplifiers.  The
//			nominal recommended setting for a 10 kohm load with 1.0 nF load cap
//			is 0x3. There are only 5 valid settings.
//				0x0=0.18 mA
//				0x1=0.27 mA
//				0x3=0.36 mA
//				0x7=0.45 mA
//				0xF=0.54 mA
// 5:0	LO_VAGCNTRL	LINEOUT Amplifier Analog Ground Voltage
//				Controls the analog ground voltage for the LINEOUT amplifiers
//				in 25 mV steps. This should usually be set to VDDIO/2.
//				0x00 = 0.800 V
//				...
//				0x1F = 1.575 V
//				...
//				0x23 = 1.675 V
//				0x24-0x3F are invalid

#define CHIP_LINE_OUT_VOL		0x002E
// 12:8	LO_VOL_RIGHT	LINEOUT Right Channel Volume (default=4)
//				Controls the right channel LINEOUT volume in 0.5 dB steps.
//				Higher codes have more attenuation.
// 4:0	LO_VOL_LEFT	LINEOUT Left Channel Output Level (default=4)
//				Used to normalize the output level of the left line output
//				to full scale based on the values used to set
//				LINE_OUT_CTRL->LO_VAGCNTRL and CHIP_REF_CTRL->VAG_VAL.
//				In general this field should be set to:
//				40*log((VAG_VAL)/(LO_VAGCNTRL)) + 15
//				Suggested values based on typical VDDIO and VDDA voltages.
//					VDDA  VAG_VAL VDDIO  LO_VAGCNTRL LO_VOL_*
//					1.8 V    0.9   3.3 V     1.55      0x06
//					1.8 V    0.9   1.8 V      0.9      0x0F
//					3.3 V   1.55   1.8 V      0.9      0x19
//					3.3 V   1.55   3.3 V     1.55      0x0F
//				After setting to the nominal voltage, this field can be used
//				to adjust the output level in +/-0.5 dB increments by using
//				values higher or lower than the nominal setting.

#define CHIP_ANA_POWER			0x0030 // power down controls for the analog blocks.
		// The only other power-down controls are BIAS_RESISTOR in the MIC_CTRL register
		//  and the EN_ZCD control bits in ANA_CTRL.
// 14	DAC_MONO	While DAC_POWERUP is set, this allows the DAC to be put into left only
//				mono operation for power savings. 0=mono, 1=stereo (default)
// 13	LINREG_SIMPLE_POWERUP  Power up the simple (low power) digital supply regulator.
//				After reset, this bit can be cleared IF VDDD is driven
//				externally OR the primary digital linreg is enabled with
//				LINREG_D_POWERUP
// 12	STARTUP_POWERUP	Power up the circuitry needed during the power up ramp and reset.
//				After reset this bit can be cleared if VDDD is coming from
//				an external source.
// 11	VDDC_CHRGPMP_POWERUP Power up the VDDC charge pump block. If neither VDDA or VDDIO
//				is 3.0 V or larger this bit should be cleared before analog
//				blocks are powered up.
// 10	PLL_POWERUP	PLL Power Up, 0 = Power down, 1 = Power up
//				When cleared, the PLL is turned off. This must be set before
//				CHIP_CLK_CTRL->MCLK_FREQ is programmed to 0x3. The
//				CHIP_PLL_CTRL register must be configured correctly before
//				setting this bit.
// 9	LINREG_D_POWERUP Power up the primary VDDD linear regulator, 0 = Power down, 1 = Power up
// 8	VCOAMP_POWERUP	Power up the PLL VCO amplifier, 0 = Power down, 1 = Power up
// 7	VAG_POWERUP	Power up the VAG reference buffer.
//				Setting this bit starts the power up ramp for the headphone
//				and LINEOUT. The headphone (and/or LINEOUT) powerup should
//				be set BEFORE clearing this bit. When this bit is cleared
//				the power-down ramp is started. The headphone (and/or LINEOUT)
//				powerup should stay set until the VAG is fully ramped down
//				(200 to 400 ms after clearing this bit).
//				0x0 = Power down, 0x1 = Power up
// 6	ADC_MONO	While ADC_POWERUP is set, this allows the ADC to be put into left only
//				mono operation for power savings. This mode is useful when
//				only using the microphone input.
//				0x0 = Mono (left only), 0x1 = Stereo
// 5	REFTOP_POWERUP	Power up the reference bias currents
//				0x0 = Power down, 0x1 = Power up
//				This bit can be cleared when the part is a sleep state
//				to minimize analog power.
// 4	HEADPHONE_POWERUP Power up the headphone amplifiers
//				0x0 = Power down, 0x1 = Power up
// 3	DAC_POWERUP	Power up the DACs
//				0x0 = Power down, 0x1 = Power up
// 2	CAPLESS_HEADPHONE_POWERUP Power up the capless headphone mode
//				0x0 = Power down, 0x1 = Power up
// 1	ADC_POWERUP	Power up the ADCs
//				0x0 = Power down, 0x1 = Power up
// 0	LINEOUT_POWERUP	Power up the LINEOUT amplifiers
//				0x0 = Power down, 0x1 = Power up

#define CHIP_PLL_CTRL			0x0032
// 15:11 INT_DIVISOR
// 10:0 FRAC_DIVISOR

#define CHIP_CLK_TOP_CTRL		0x0034
// 11	ENABLE_INT_OSC	Setting this bit enables an internal oscillator to be used for the
//				zero cross detectors, the short detect recovery, and the
//				charge pump. This allows the I2S clock to be shut off while
//				still operating an analog signal path. This bit can be kept
//				on when the I2S clock is enabled, but the I2S clock is more
//				accurate so it is preferred to clear this bit when I2S is present.
// 3	INPUT_FREQ_DIV2	SYS_MCLK divider before PLL input
//				0x0 = pass through
//				0x1 = SYS_MCLK is divided by 2 before entering PLL
//				This must be set when the input clock is above 17 Mhz. This
//				has no effect when the PLL is powered down.

#define CHIP_ANA_STATUS			0x0036
// 9	LRSHORT_STS	This bit is high whenever a short is detected on the left or right
//				channel headphone drivers.
// 8	CSHORT_STS	This bit is high whenever a short is detected on the capless headphone
//				common/center channel driver.
// 4	PLL_IS_LOCKED	This bit goes high after the PLL is locked.

#define CHIP_ANA_TEST1			0x0038 //  intended only for debug.
#define CHIP_ANA_TEST2			0x003A //  intended only for debug.

#define CHIP_SHORT_CTRL			0x003C
// 14:12 LVLADJR	Right channel headphone	short detector in 25 mA steps.
//				0x3=25 mA
//				0x2=50 mA
//				0x1=75 mA
//				0x0=100 mA
//				0x4=125 mA
//				0x5=150 mA
//				0x6=175 mA
//				0x7=200 mA
//				This trip point can vary by ~30% over process so leave plenty
//				of guard band to avoid false trips.  This short detect trip
//				point is also effected by the bias current adjustments made
//				by CHIP_REF_CTRL->BIAS_CTRL and by CHIP_ANA_TEST1->HP_IALL_ADJ.
// 10:8	LVLADJL		Left channel headphone short detector in 25 mA steps.
//				(same scale as LVLADJR)
// 6:4	LVLADJC		Capless headphone center channel short detector in 50 mA steps.
//				0x3=50 mA
//				0x2=100 mA
//				0x1=150 mA
//				0x0=200 mA
//				0x4=250 mA
//				0x5=300 mA
//				0x6=350 mA
//				0x7=400 mA
// 3:2	MODE_LR		Behavior of left/right short detection
//				0x0 = Disable short detector, reset short detect latch,
//					software view non-latched short signal
//				0x1 = Enable short detector and reset the latch at timeout
//					(every ~50 ms)
//				0x2 = This mode is not used/invalid
//				0x3 = Enable short detector with only manual reset (have
//					to return to 0x0 to reset the latch)
// 1:0	MODE_CM		Behavior of capless headphone central short detection
//				(same settings as MODE_LR)

#define DAP_CONTROL			0x0100
#define DAP_PEQ				0x0102
#define DAP_BASS_ENHANCE		0x0104
#define DAP_BASS_ENHANCE_CTRL		0x0106
#define DAP_AUDIO_EQ			0x0108
#define DAP_SGTL_SURROUND		0x010A
#define DAP_FILTER_COEF_ACCESS		0x010C
#define DAP_COEF_WR_B0_MSB		0x010E
#define DAP_COEF_WR_B0_LSB		0x0110
#define DAP_AUDIO_EQ_BASS_BAND0		0x0116 // 115 Hz
#define DAP_AUDIO_EQ_BAND1		0x0118 // 330 Hz
#define DAP_AUDIO_EQ_BAND2		0x011A // 990 Hz
#define DAP_AUDIO_EQ_BAND3		0x011C // 3000 Hz
#define DAP_AUDIO_EQ_TREBLE_BAND4	0x011E // 9900 Hz
#define DAP_MAIN_CHAN			0x0120
#define DAP_MIX_CHAN			0x0122
#define DAP_AVC_CTRL			0x0124
#define DAP_AVC_THRESHOLD		0x0126
#define DAP_AVC_ATTACK			0x0128
#define DAP_AVC_DECAY			0x012A
#define DAP_COEF_WR_B1_MSB		0x012C
#define DAP_COEF_WR_B1_LSB		0x012E
#define DAP_COEF_WR_B2_MSB		0x0130
#define DAP_COEF_WR_B2_LSB		0x0132
#define DAP_COEF_WR_A1_MSB		0x0134
#define DAP_COEF_WR_A1_LSB		0x0136
#define DAP_COEF_WR_A2_MSB		0x0138
#define DAP_COEF_WR_A2_LSB		0x013A

#define SGTL5000_I2C_ADDR_CS_LOW	0x0A  // CTRL_ADR0_CS pin low (normal configuration)
#define SGTL5000_I2C_ADDR_CS_HIGH	0x2A // CTRL_ADR0_CS  pin high


void AudioControlSGTL5000::setAddress(uint8_t level)
{
	if (level == LOW) {
		i2c_addr = SGTL5000_I2C_ADDR_CS_LOW;
	} else {
		i2c_addr = SGTL5000_I2C_ADDR_CS_HIGH;
	}
}

bool AudioControlSGTL5000::enable(void)
{
	muted = true;
	Wire.begin();
	delay(5);
	//Serial.print("chip ID = ");
	//delay(5);
	//unsigned int n = this->read(CHIP_ID);
	//Serial.println(n, HEX);

	this->write(CHIP_ANA_POWER, 0x4060);  // VDDD is externally driven with 1.8V
	this->write(CHIP_LINREG_CTRL, 0x006C);  // VDDA & VDDIO both over 3.1V
	this->write(CHIP_REF_CTRL, 0x01F2); // VAG=1.575, normal ramp, +12.5% bias current
	this->write(CHIP_LINE_OUT_CTRL, 0x0F22); // LO_VAGCNTRL=1.65V, OUT_CURRENT=0.54mA
	this->write(CHIP_SHORT_CTRL, 0x4446);  // allow up to 125mA
	this->write(CHIP_ANA_CTRL, 0x0137);  // enable zero cross detectors
	this->write(CHIP_ANA_POWER, 0x40FF); // power up: lineout, hp, adc, dac
	this->write(CHIP_DIG_POWER, 0x0073); // power up all digital stuff
	delay(400);
	this->write(CHIP_LINE_OUT_VOL, 0x1D1D); // default approx 1.3 volts peak-to-peak
	this->write(CHIP_CLK_CTRL, 0x0004);  // 44.1 kHz, 256*Fs
	this->write(CHIP_I2S_CTRL, 0x0130); // SCLK=32*Fs, 16bit, I2S format
	// default signal routing is ok?
	this->write(CHIP_SSS_CTRL, 0x0010); // ADC->I2S, I2S->DAC
	this->write(CHIP_ADCDAC_CTRL, 0x0000); // disable dac mute
	this->write(CHIP_DAC_VOL, 0x3C3C); // digital gain, 0dB
	this->write(CHIP_ANA_HP_CTRL, 0x7F7F); // set volume (lowest level)
	this->write(CHIP_ANA_CTRL, 0x0036);  // enable zero cross detectors
	//mute = false;
	semi_automated = true;
	return true;
}

unsigned int AudioControlSGTL5000::read(unsigned int reg)
{
	unsigned int val;
	Wire.beginTransmission(i2c_addr);
	Wire.write(reg >> 8);
	Wire.write(reg);
	if (Wire.endTransmission(false) != 0) return 0;
	if (Wire.requestFrom((int)i2c_addr, 2) < 2) return 0;
	val = Wire.read() << 8;
	val |= Wire.read();
	return val;
}

bool AudioControlSGTL5000::write(unsigned int reg, unsigned int val)
{
	if (reg == CHIP_ANA_CTRL) ana_ctrl = val;
	Wire.beginTransmission(i2c_addr);
	Wire.write(reg >> 8);
	Wire.write(reg);
	Wire.write(val >> 8);
	Wire.write(val);
	if (Wire.endTransmission() == 0) return true;
	return false;
}

unsigned int AudioControlSGTL5000::modify(unsigned int reg, unsigned int val, unsigned int iMask)
{
	unsigned int val1 = (this->read(reg)&(~iMask))|val;
	if(!this->write(reg,val1)) return 0;
	return val1;
}

bool AudioControlSGTL5000::volumeInteger(unsigned int n)
{
	if (n == 0) {
		muted = true;
		this->write(CHIP_ANA_HP_CTRL, 0x7F7F);
		return muteHeadphone();
	} else if (n > 0x80) {
		n = 0;
	} else {
		n = 0x80 - n;
	}
	if (muted) {
		muted = false;
		unmuteHeadphone();
	}
	n = n | (n << 8);
	return this->write(CHIP_ANA_HP_CTRL, n);  // set volume
}

bool AudioControlSGTL5000::volume(float left, float right)
{
	unsigned short m=((0x7F-calcVol(right,0x7F))<<8)|(0x7F-calcVol(left,0x7F));
	return this->write(CHIP_ANA_HP_CTRL, m);
}

bool AudioControlSGTL5000::micGain(unsigned int dB)
{
	unsigned int preamp_gain, input_gain;

	if (dB >= 40) {
		preamp_gain = 3;
		dB -= 40;
	} else if (dB >= 30) {
		preamp_gain = 2;
		dB -= 30;
	} else if (dB >= 20) {
		preamp_gain = 1;
		dB -= 20;
	} else {
		preamp_gain = 0;
	}
	input_gain = (dB * 2) / 3;
	if (input_gain > 15) input_gain = 15;

	return this->write(CHIP_MIC_CTRL, 0x0170 | preamp_gain)
	    && this->write(CHIP_ANA_ADC_CTRL, (input_gain << 4) | input_gain);
}

// CHIP_ANA_ADC_CTRL
// Actual measured full-scale peak-to-peak sine wave input for max signal
//  0: 3.12 Volts p-p
//  1: 2.63 Volts p-p
//  2: 2.22 Volts p-p
//  3: 1.87 Volts p-p
//  4: 1.58 Volts p-p
//  5: 1.33 Volts p-p
//  6: 1.11 Volts p-p
//  7: 0.94 Volts p-p
//  8: 0.79 Volts p-p
//  9: 0.67 Volts p-p
// 10: 0.56 Volts p-p
// 11: 0.48 Volts p-p
// 12: 0.40 Volts p-p
// 13: 0.34 Volts p-p
// 14: 0.29 Volts p-p
// 15: 0.24 Volts p-p
bool AudioControlSGTL5000::lineInLevel(uint8_t left, uint8_t right)
{
	if (left > 15) left = 15;
	if (right > 15) right = 15;
	return this->write(CHIP_ANA_ADC_CTRL, (left << 4) | right);
}

// CHIP_LINE_OUT_VOL
//  Actual measured full-scale peak-to-peak sine wave output voltage:
//  0-12: output has clipping
//  13: 3.16 Volts p-p
//  14: 2.98 Volts p-p
//  15: 2.83 Volts p-p
//  16: 2.67 Volts p-p
//  17: 2.53 Volts p-p
//  18: 2.39 Volts p-p
//  19: 2.26 Volts p-p
//  20: 2.14 Volts p-p
//  21: 2.02 Volts p-p
//  22: 1.91 Volts p-p
//  23: 1.80 Volts p-p
//  24: 1.71 Volts p-p
//  25: 1.62 Volts p-p
//  26: 1.53 Volts p-p
//  27: 1.44 Volts p-p
//  28: 1.37 Volts p-p
//  29: 1.29 Volts p-p
//  30: 1.22 Volts p-p
//  31: 1.16 Volts p-p
unsigned short AudioControlSGTL5000::lineOutLevel(uint8_t n)
{
	if (n > 31) n = 31;
	else if (n < 13) n = 13;
	return modify(CHIP_LINE_OUT_VOL,(n<<8)|n,(31<<8)|31);
}

unsigned short AudioControlSGTL5000::lineOutLevel(uint8_t left, uint8_t right)
{
	if (left > 31) left = 31;
	else if (left < 13) left = 13;
	if (right > 31) right = 31;
	else if (right < 13) right = 13;
	return modify(CHIP_LINE_OUT_VOL,(right<<8)|left,(31<<8)|31);
}

unsigned short AudioControlSGTL5000::dacVolume(float n) // set both directly
{
	if ((this->read(CHIP_ADCDAC_CTRL)&(3<<2)) != ((n>0 ? 0:3)<<2)) {
		modify(CHIP_ADCDAC_CTRL,(n>0 ? 0:3)<<2,3<<2);
	}
	unsigned char m=calcVol(n,0xC0);
	return modify(CHIP_DAC_VOL,((0xFC-m)<<8)|(0xFC-m),65535);
}
unsigned short AudioControlSGTL5000::dacVolume(float left, float right)
{
	unsigned short adcdac=((right>0 ? 0:2)|(left>0 ? 0:1))<<2;
	if ((this->read(CHIP_ADCDAC_CTRL)&(3<<2)) != adcdac) {
		modify(CHIP_ADCDAC_CTRL,adcdac,1<<2);
	}
	unsigned short m=(0xFC-calcVol(right,0xC0))<<8|(0xFC-calcVol(left,0xC0));
	return modify(CHIP_DAC_VOL,m,65535);
}

bool AudioControlSGTL5000::dacVolumeRamp()
{
	return modify(CHIP_ADCDAC_CTRL, 0x300, 0x300);
}

bool AudioControlSGTL5000::dacVolumeRampLinear()
{
	return modify(CHIP_ADCDAC_CTRL, 0x200, 0x300);
}

bool AudioControlSGTL5000::dacVolumeRampDisable()
{
	return modify(CHIP_ADCDAC_CTRL, 0, 0x300);
}




unsigned short AudioControlSGTL5000::adcHighPassFilterEnable(void)
{
	return modify(CHIP_ADCDAC_CTRL, 0, 3);
}

unsigned short AudioControlSGTL5000::adcHighPassFilterFreeze(void)
{
	return modify(CHIP_ADCDAC_CTRL, 2, 3);
}

unsigned short AudioControlSGTL5000::adcHighPassFilterDisable(void)
{
	return modify(CHIP_ADCDAC_CTRL, 1, 3);
}


// DAP_CONTROL

unsigned short AudioControlSGTL5000::audioPreProcessorEnable(void)
{
	// audio processor used to pre-process analog input before Teensy
	return this->write(DAP_CONTROL, 1) && this->write(CHIP_SSS_CTRL, 0x0013);
}

unsigned short AudioControlSGTL5000::audioPostProcessorEnable(void)
{
	// audio processor used to post-process Teensy output before headphones/lineout
	return this->write(DAP_CONTROL, 1) && this->write(CHIP_SSS_CTRL, 0x0070);
}

unsigned short AudioControlSGTL5000::audioProcessorDisable(void)
{
	return this->write(CHIP_SSS_CTRL, 0x0010) && this->write(DAP_CONTROL, 0);
}


// DAP_PEQ
unsigned short AudioControlSGTL5000::eqFilterCount(uint8_t n) // valid to n&7, 0 thru 7 filters enabled.
{
	return modify(DAP_PEQ,(n&7),7);
}

// DAP_AUDIO_EQ
unsigned short AudioControlSGTL5000::eqSelect(uint8_t n) // 0=NONE, 1=PEQ (7 IIR Biquad filters), 2=TONE (tone), 3=GEQ (5 band EQ)
{
	return modify(DAP_AUDIO_EQ,n&3,3);
}

unsigned short AudioControlSGTL5000::eqBand(uint8_t bandNum, float n)
{
	if(semi_automated) automate(1,3);
	return dap_audio_eq_band(bandNum, n);
}
void AudioControlSGTL5000::eqBands(float bass, float mid_bass, float midrange, float mid_treble, float treble)
{
	if(semi_automated) automate(1,3);
	dap_audio_eq_band(0,bass);
	dap_audio_eq_band(1,mid_bass);
	dap_audio_eq_band(2,midrange);
	dap_audio_eq_band(3,mid_treble);
	dap_audio_eq_band(4,treble);
}
void AudioControlSGTL5000::eqBands(float bass, float treble) // dap_audio_eq(2);
{
	if(semi_automated) automate(1,2);
	dap_audio_eq_band(0,bass);
	dap_audio_eq_band(4,treble);
}

// SGTL5000 PEQ Coefficient loader
void AudioControlSGTL5000::eqFilter(uint8_t filterNum, int *filterParameters)
{
	// TODO: add the part that selects 7 PEQ filters.
	if(semi_automated) automate(1,1,filterNum+1);
	modify(DAP_FILTER_COEF_ACCESS,(uint16_t)filterNum,15); 
	this->write(DAP_COEF_WR_B0_MSB,(*filterParameters>>4)&65535);
	this->write(DAP_COEF_WR_B0_LSB,(*filterParameters++)&15);
	this->write(DAP_COEF_WR_B1_MSB,(*filterParameters>>4)&65535);
	this->write(DAP_COEF_WR_B1_LSB,(*filterParameters++)&15);
	this->write(DAP_COEF_WR_B2_MSB,(*filterParameters>>4)&65535);
	this->write(DAP_COEF_WR_B2_LSB,(*filterParameters++)&15);
	this->write(DAP_COEF_WR_A1_MSB,(*filterParameters>>4)&65535);
	this->write(DAP_COEF_WR_A1_LSB,(*filterParameters++)&15);
	this->write(DAP_COEF_WR_A2_MSB,(*filterParameters>>4)&65535);
	this->write(DAP_COEF_WR_A2_LSB,(*filterParameters++)&15);
	this->write(DAP_FILTER_COEF_ACCESS,(uint16_t)0x100|filterNum);
}

/* Valid values for dap_avc parameters

	maxGain; Maximum gain that can be applied
	0 - 0 dB
	1 - 6.0 dB
	2 - 12 dB
	
	lbiResponse; Integrator Response
	0 - 0 mS
	1 - 25 mS
	2 - 50 mS
	3 - 100 mS
	
	hardLimit
	0 - Hard limit disabled. AVC Compressor/Expander enabled.
	1 - Hard limit enabled. The signal is limited to the programmed threshold (signal saturates at the threshold)
	
	threshold
	floating point in range 0 to -96 dB
	
	attack
	floating point figure is dB/s rate at which gain is increased
	
	decay
	floating point figure is dB/s rate at which gain is reduced
*/
unsigned short AudioControlSGTL5000::autoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay)
{
	//if(semi_automated&&(!read(DAP_CONTROL)&1)) audioProcessorEnable();
	if(maxGain>2) maxGain=2;
	lbiResponse&=3;
	hardLimit&=1;
	uint8_t thresh=(pow(10,threshold/20)*0.636)*pow(2,15);
	uint8_t att=(1-pow(10,-(attack/(20*44100))))*pow(2,19);
	uint8_t dec=(1-pow(10,-(decay/(20*44100))))*pow(2,23);
	this->write(DAP_AVC_THRESHOLD,thresh);
	this->write(DAP_AVC_ATTACK,att);
	this->write(DAP_AVC_DECAY,dec);
	return 	modify(DAP_AVC_CTRL,maxGain<<12|lbiResponse<<8|hardLimit<<5,3<<12|3<<8|1<<5);
}
unsigned short AudioControlSGTL5000::autoVolumeEnable(void)
{
	return modify(DAP_AVC_CTRL, 1, 1);
}
unsigned short AudioControlSGTL5000::autoVolumeDisable(void)
{
	return modify(DAP_AVC_CTRL, 0, 1);
}

unsigned short AudioControlSGTL5000::enhanceBass(float lr_lev, float bass_lev)
{
	return modify(DAP_BASS_ENHANCE_CTRL,((0x3F-calcVol(lr_lev,0x3F))<<8) | (0x7F-calcVol(bass_lev,0x7F)), (0x3F<<8) | 0x7F);
}
unsigned short AudioControlSGTL5000::enhanceBass(float lr_lev, float bass_lev, uint8_t hpf_bypass, uint8_t cutoff)
{
	modify(DAP_BASS_ENHANCE,(hpf_bypass&1)<<8|(cutoff&7)<<4,1<<8|7<<4);
	return enhanceBass(lr_lev,bass_lev);
}
unsigned short AudioControlSGTL5000::enhanceBassEnable(void)
{
	return modify(DAP_BASS_ENHANCE, 1, 1);
}
unsigned short AudioControlSGTL5000::enhanceBassDisable(void)
{
	return modify(DAP_BASS_ENHANCE, 0, 1);
}
unsigned short AudioControlSGTL5000::surroundSound(uint8_t width)
{
	return modify(DAP_SGTL_SURROUND,(width&7)<<4,7<<4);
}
unsigned short AudioControlSGTL5000::surroundSound(uint8_t width, uint8_t select)
{
	return modify(DAP_SGTL_SURROUND,((width&7)<<4)|(select&3), (7<<4)|3);
}
unsigned short AudioControlSGTL5000::surroundSoundEnable(void)
{
	return modify(DAP_SGTL_SURROUND, 3, 3);
}
unsigned short AudioControlSGTL5000::surroundSoundDisable(void)
{
	return modify(DAP_SGTL_SURROUND, 0, 3);
}

unsigned char AudioControlSGTL5000::calcVol(float n, unsigned char range)
{
	// n=(n*(((float)range)/100))+0.499;
	n=(n*(float)range)+0.499;
	if ((unsigned char)n>range) n=range;
	return (unsigned char)n;
}

// DAP_AUDIO_EQ_BASS_BAND0 & DAP_AUDIO_EQ_BAND1 & DAP_AUDIO_EQ_BAND2 etc etc
unsigned short AudioControlSGTL5000::dap_audio_eq_band(uint8_t bandNum, float n) // by signed percentage -100/+100; dap_audio_eq(3);
{
	n=(n*48)+0.499;
	if(n<-47) n=-47;
	if(n>48) n=48;
	n+=47;
	return modify(DAP_AUDIO_EQ_BASS_BAND0+(bandNum*2),(unsigned int)n,127);
}

void AudioControlSGTL5000::automate(uint8_t dap, uint8_t eq)
{
	//if((dap!=0)&&(!(read(DAP_CONTROL)&1))) audioProcessorEnable();
	if((this->read(DAP_AUDIO_EQ)&3) != eq) eqSelect(eq);
}

void AudioControlSGTL5000::automate(uint8_t dap, uint8_t eq, uint8_t filterCount)
{
	automate(dap,eq);
	if (filterCount > (this->read(DAP_PEQ)&7)) eqFilterCount(filterCount);
}


// if(SGTL5000_PEQ) quantization_unit=524288; if(AudioFilterBiquad) quantization_unit=2147483648;
void calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef)
{

// I used resources like http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
// to make this routine, I tested most of the filter types and they worked. Such filters have limits and
// before calling this routine with varying values the end user should check that those values are limited
// to valid results.

  float A;
  if(filtertype<FILTER_PARAEQ) A=pow(10,dB_Gain/20); else A=pow(10,dB_Gain/40);
  float W0 = 2*3.14159265358979323846*fC/fS; 
  float cosw=cosf(W0);
  float sinw=sinf(W0);
  //float alpha = sinw*sinh((log(2)/2)*BW*W0/sinw);
  //float beta = sqrt(2*A);
  float alpha = sinw / (2 * Q); 
  float beta = sqrtf(A)/Q;
  float b0,b1,b2,a0,a1,a2;

  switch(filtertype) {
  case FILTER_LOPASS:
    b0 = (1.0F - cosw) * 0.5F; // =(1-COS($H$2))/2
    b1 = 1.0F - cosw;
    b2 = (1.0F - cosw) * 0.5F;
    a0 = 1.0F + alpha;
    a1 = 2.0F * cosw;
    a2 = alpha - 1.0F;
  break;
  case FILTER_HIPASS:
    b0 = (1.0F + cosw) * 0.5F;
    b1 = -(cosw + 1.0F);
    b2 = (1.0F + cosw) * 0.5F;
    a0 = 1.0F + alpha;
    a1 = 2.0F * cosw;
    a2 = alpha - 1.0F;
  break;
  case FILTER_BANDPASS:
    b0 = alpha;
    b1 = 0.0F;
    b2 = -alpha;
    a0 = 1.0F + alpha;
    a1 = 2.0F * cosw;
    a2 = alpha - 1.0F;
   break;
  case FILTER_NOTCH:
    b0=1;
    b1=-2*cosw;
    b2=1;
    a0=1+alpha;
    a1=2*cosw;
    a2=-(1-alpha);
  break;
  case FILTER_PARAEQ:
    b0 = 1 + (alpha*A);
    b1 =-2 * cosw;
    b2 = 1 - (alpha*A);
    a0 = 1 + (alpha/A);
    a1 = 2 * cosw;
    a2 =-(1-(alpha/A));
  break;
  case FILTER_LOSHELF:
    b0 = A * ((A+1.0F) - ((A-1.0F)*cosw) + (beta*sinw));
    b1 = 2.0F * A * ((A-1.0F) - ((A+1.0F)*cosw));
    b2 = A * ((A+1.0F) - ((A-1.0F)*cosw) - (beta*sinw));
    a0 = (A+1.0F) + ((A-1.0F)*cosw) + (beta*sinw);
    a1 = 2.0F * ((A-1.0F) + ((A+1.0F)*cosw));
    a2 = -((A+1.0F) + ((A-1.0F)*cosw) - (beta*sinw));
  break;
  case FILTER_HISHELF:
    b0 = A * ((A+1.0F) + ((A-1.0F)*cosw) + (beta*sinw));
    b1 = -2.0F * A * ((A-1.0F) + ((A+1.0F)*cosw));
    b2 = A * ((A+1.0F) + ((A-1.0F)*cosw) - (beta*sinw));
    a0 = (A+1.0F) - ((A-1.0F)*cosw) + (beta*sinw);
    a1 = -2.0F * ((A-1.0F) - ((A+1.0F)*cosw));
    a2 = -((A+1.0F) - ((A-1.0F)*cosw) - (beta*sinw));
  default:
    b0 = 0.5;
    b1 = 0.0;
    b2 = 0.0;
    a0 = 1.0;
    a1 = 0.0;
    a2 = 0.0;
  }

  a0=(a0*2)/(float)quantization_unit; // once here instead of five times there...
  b0/=a0;
  *coef++=(int)(b0+0.499);
  b1/=a0;
  *coef++=(int)(b1+0.499);
  b2/=a0;
  *coef++=(int)(b2+0.499);
  a1/=a0;
  *coef++=(int)(a1+0.499);
  a2/=a0;
  *coef++=(int)(a2+0.499);
}


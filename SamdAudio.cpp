/*
  Copyright (c) 2016 Th�o Meyer aka AloyseTech. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "SamdAudio.h"

//for using flash memory as WAV file storage
#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>
extern Adafruit_W25Q16BV_FatFs memory;

//for using SD card as WAV file storage
//#include <SdFat.h>
//extern SdFat memory;

//*********************************************************************
//Global variables
File __audioFile[MAX_N_CHANNELS];
volatile bool __audioPlaying[MAX_N_CHANNELS]={false};
volatile bool __audioFileReady[MAX_N_CHANNELS] = {false};
volatile uint32_t __SampleIndex[MAX_N_CHANNELS];
uint8_t __WavSamples[MAX_N_CHANNELS][2][MAX_AUDIO_BUFFER_SIZE];
uint8_t rampDivisor[MAX_N_CHANNELS]={1};
uint8_t whichBuffer[MAX_N_CHANNELS]={0};
volatile bool fillNextBuffer[MAX_N_CHANNELS]={true};
volatile uint16_t __audioData;


int __Volume;
bool __criticalSection = false;

int __numOfChannelsUsed = 4;
int __audioBufferSize = 1024;

// unused declarations, commented out
void TC5_Handler (void) __attribute__ ((weak, alias("AudioPlay_Handler")));
void TC3_Handler (void) __attribute__ ((weak, alias("AudioRead_Handler")));


//*********************************************************************


int SamdAudio::begin(uint32_t sampleRate, uint8_t numOfChannels, uint16_t audio_buffer_size)
{
  __audioBufferSize = audio_buffer_size;


	if(numOfChannels == 1 || numOfChannels == 2 || numOfChannels == 4)
	{
		// initialize the global variable with the channels to use
		__numOfChannelsUsed = numOfChannels;
	}
	else
	{
		// bad input passed, assume all 4 channels to be safe
		__numOfChannelsUsed = 4;
	}

	// initialize arrays
    for(uint8_t index=0; index<MAX_N_CHANNELS; index++)
    {
        __audioFileReady[index]=false;
        __SampleIndex[index]=0;
    }

    /*Modules configuration */
    dacConfigure();
    configurePlayerTimer(sampleRate);
    configureReaderTimer();

    return 0;
}

void SamdAudio::end() {
    disablePlayerTimer();
    resetPlayerTimer();
    disableReaderTimer();
    analogWrite(A0, 0);
}


void SamdAudio::stopChannel(uint8_t c)
{
	//if(c>=0 && c<__numOfChannelsUsed) //unsigned, cant be less than zero
    if(c<__numOfChannelsUsed)
    {
        __audioFileReady[c]=false;
        __audioFile[c].close();
        __SampleIndex[c]=0;
        __audioPlaying[c]=false;
    }
}

void SamdAudio::play(const char *fname, uint8_t channel) {

	//if(channel<0 || channel>=__numOfChannelsUsed)//unsigned, cant be negative
    if(channel>=__numOfChannelsUsed)
        return;

    disableReaderTimer();
    if(__audioFileReady[channel])
        __audioFile[channel].close();
    __audioFile[channel] = memory.open(fname);
    if(!__audioFile[channel]){
        //end();
        //SerialUSB.println("Error opening file");
        return;
    }


    whichBuffer[channel]=0;
    fillNextBuffer[channel]=1;

    __audioFile[channel].read(__WavSamples[channel][whichBuffer[channel]], __audioBufferSize);

    __SampleIndex[channel]=0;
    __audioFileReady[channel] = true;

    rampDivisor[channel]=RAMPIN;

    /*once the buffer is filled for the first time the counter can be started*/
    if(alonePlaying(channel))
    {
        enablePlayerTimer();
    }
    __audioPlaying[channel]=true;

    enableReaderTimer();
}

bool SamdAudio::alonePlaying(uint8_t channel)
{
    for(uint8_t index=0; index<__numOfChannelsUsed; index++)
    {
        if(index!=channel && __audioPlaying[index])
            return false;
    }
    return true;
}

bool __channelsPlaying()
{
    for(uint8_t index=0; index<__numOfChannelsUsed; index++)
    {
        if(__audioPlaying[index])
            return true;
    }
    return false;
}

/**
 * Configures the DAC in event triggered mode.
 *
 * Configures the DAC to use the module's default configuration, with output
 * channel mode configured for event triggered conversions.
 */
void SamdAudio::dacConfigure(void){
    analogWriteResolution(10);
    //    analogWrite(A0, 0);

    DAC->CTRLA.bit.ENABLE = 0x01;
    DAC->DATA.reg = 0;
    while (DAC->STATUS.bit.SYNCBUSY == 1);
}

/**
 * Configures the TC to generate output events at the sample frequency.
 *
 * Configures the TC in Frequency Generation mode, with an event output once
 * each time the audio sample frequency period expires.
 */
void SamdAudio::configurePlayerTimer(uint32_t sampleRate)
{
    // Enable GCLK for TCC2 and TC5 (timer counter input clock)
    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
    while (GCLK->STATUS.bit.SYNCBUSY);

    resetPlayerTimer();

    // Set Timer counter Mode to 16 bits
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;

    // Set TC5 mode as match frequency
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;

    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1 | TC_CTRLA_ENABLE;

    TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sampleRate - 1);
    while (syncPlayerTimer());

    // Configure interrupt request
    NVIC_DisableIRQ(TC5_IRQn);
    NVIC_ClearPendingIRQ(TC5_IRQn);
    NVIC_SetPriority(TC5_IRQn, 0);
    NVIC_EnableIRQ(TC5_IRQn);

    // Enable the TC5 interrupt request
    TC5->COUNT16.INTENSET.bit.MC0 = 1;
    while (syncPlayerTimer());
}


bool SamdAudio::syncPlayerTimer()
{
    return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

void SamdAudio::enablePlayerTimer()
{
    // Enable TC
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (syncPlayerTimer());
}

void SamdAudio::resetPlayerTimer()
{
    // Reset TCx
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (syncPlayerTimer());
    while (TC5->COUNT16.CTRLA.bit.SWRST);
}

void SamdAudio::disablePlayerTimer()
{
    // Disable TC5
    TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (syncPlayerTimer());
}


void SamdAudio::criticalON() {
    __criticalSection = true;
}

void SamdAudio::criticalOFF() {
    __criticalSection = false;
}

//SamdAudio AudioPlayer;


void SamdAudio::configureReaderTimer()
{
    // The GCLK clock provider to use
    // GCLK0, GCLK1 & GCLK3 are used already, see startup.c

    //GCLK0 ...  freezes serial.. on adafruit M0

    const uint8_t GCLK_SRC = 1; //1 works for Adafruit M0 express

    // Configure the XOSC32K to run in standby
    //SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;

    // Setup clock provider GCLK_SRC with a /2 source divider
    // GCLK_GENDIV_ID(X) specifies which GCLK we are configuring
    // GCLK_GENDIV_DIV(Y) specifies the clock prescalar / divider
    // If GENCTRL.DIVSEL is set (see further below) the divider
    // is 2^(Y+1). If GENCTRL.DIVSEL is 0, the divider is simply Y
    // This register has to be written in a single operation
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(GCLK_SRC) | GCLK_GENDIV_DIV(2);
    while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }

    // Configure the GCLK module
    // GCLK_GENCTRL_GENEN, enable the specific GCLK module
    // GCLK_GENCTRL_SRC_XOSC32K, set the source to the XOSC32K
    // GCLK_GENCTRL_ID(X), specifies which GCLK we are configuring
    // GCLK_GENCTRL_DIVSEL, specify which prescalar mode we are using
    // GCLK_RUNSTDBY, keep the GCLK running when in standby mode
    // Output from this module is 16khz (32khz / 2)
    // This register has to be written in a single operation.
    GCLK->GENCTRL.reg = GCLK_GENCTRL_GENEN |
    GCLK_GENCTRL_SRC_XOSC32K |
    GCLK_GENCTRL_ID(GCLK_SRC);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }

    // Turn the power to the TC3 module on
    PM->APBCMASK.reg |= PM_APBCMASK_TC3;

    // Set TC3 (shared with TCC2) GCLK source to GCLK_SRC
    // GCLK_CLKCTRL_CLKEN, enable the generic clock
    // GCLK_CLKCTRL_GEN(X), specifies the GCLK generator source
    // GCLK_CLKCTRL_ID(X), specifies which generic clock we are configuring
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |
    GCLK_CLKCTRL_GEN(GCLK_SRC) |
    GCLK_CLKCTRL_ID(GCM_TCC2_TC3);
    while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }

    // Disable TC3. This is required (if enabled already)
    // before setting certain registers
    TC3->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }

    // Set the mode to 8 bit and set it to run in standby
    // TC_CTRLA_MODE_COUNT8, specify 8bit mode
    // TC_CTRLA_RUNSTDBY, keep the module running when in standby
    // TC_CTRLA_PRESCALER_DIVxx, set the prescalar to 64
    // Prescalar options include: DIV1, DIV2, DIV4, DIV8,
    // DIV16, DIV64, DIV256, DIV1024
    TC3->COUNT8.CTRLA.reg = TC_CTRLA_MODE_COUNT8 |
    TC_CTRLA_RUNSTDBY |
    TC_CTRLA_PRESCALER_DIV1;
    while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }

    // Enable the TC3 interrupt vector
    // Set the priority to second (less important than feeding the DAC
    NVIC_DisableIRQ(TC3_IRQn);
    NVIC_ClearPendingIRQ(TC3_IRQn);
    NVIC_SetPriority(TC3_IRQn, 0xFFFF);
    NVIC_EnableIRQ(TC3_IRQn);


    // Enable interrupt on overflow
    // TC_INTENSET_OVF, enable an interrupt on overflow
    TC3->COUNT8.INTENSET.reg = TC_INTENSET_OVF;
    while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }

    // Enable TC3
    TC3->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }
}

void SamdAudio::enableReaderTimer()
{
    NVIC_EnableIRQ(TC3_IRQn);
    TC3->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE;
    while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }
}

void SamdAudio::disableReaderTimer()
{
    TC3->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
        /* Wait for synchronization */
    }
    NVIC_DisableIRQ(TC3_IRQn);
}





#ifdef __cplusplus
extern "C" {
#endif

	// TC5 ISR
    void AudioPlay_Handler (void)
    {
        __audioData=0;

        for(uint8_t index=0; index<__numOfChannelsUsed; index++)
        {
            if (__audioPlaying[index])
            {
                if(__audioFile[index].available())
                {
                    if (__SampleIndex[index] < __audioBufferSize - 1)
                    {
                        __audioData+=__WavSamples[index][whichBuffer[index]][__SampleIndex[index]++]/rampDivisor[index];

                    }
                    else //last sample from buffer
                    {
                        __audioData+=__WavSamples[index][whichBuffer[index]][__SampleIndex[index]++]/rampDivisor[index];

                        __SampleIndex[index] = 0;
                        if(!fillNextBuffer[index]) //we have been able to load next buffer : continue; else, loop the buffer...
                            whichBuffer[index]=1-whichBuffer[index];
                        fillNextBuffer[index]=1;
                    }

                    if(rampDivisor[index]>1)
                        rampDivisor[index]--;
                }

            //end of file, now play ramp out
                else if (__audioFileReady[index])
                {
                    __audioFile[index].close();
                    __audioFileReady[index] = false;
                    rampDivisor[index]=__WavSamples[index][whichBuffer[index]][__SampleIndex[index]]; //start ramp out from last audio sample
                    __audioData+=rampDivisor[index];
                }
                else if(rampDivisor[index]>0)//ramp out finish, end of activity on the channel
                {
                    __audioData+=rampDivisor[index]--;
                }
                else
                {
                    __audioPlaying[index]=false;

                    if(!__channelsPlaying())
                    {
                        //tc disable
                        TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
                        while (TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY);

                        TC3->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
                        while (TC3->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY) {
                            /* Wait for synchronization */
                        }

                    }
                }
            }
        }


        if(__channelsPlaying())
        {
			if(__numOfChannelsUsed == 4 || __numOfChannelsUsed == 3)
			{
				__audioData>>=0;
			}
			else if(__numOfChannelsUsed == 2)
			{
				__audioData<<=1;
			}
			else if(__numOfChannelsUsed == 1)
			{
				__audioData<<=2;
			}

            DAC->DATA.reg = __audioData & 0x3FF; // DAC on 10 bits.
            while (DAC->STATUS.bit.SYNCBUSY == 1);
        }


        // Clear the interrupt
        TC5->COUNT16.INTFLAG.bit.MC0 = 1;

    }



    // TC3 ISR
    void AudioRead_Handler()
    {
        if (TC3->COUNT8.INTFLAG.bit.OVF)
        {
            for(uint8_t index=0; index<__numOfChannelsUsed; index++)
            {
                if(__audioPlaying[index])
                {
                    if(__audioFile[index].available() && !__criticalSection && fillNextBuffer[index])
                    {
                        __audioFile[index].read(__WavSamples[index][1-whichBuffer[index]], __audioBufferSize);
                        fillNextBuffer[index]=0;
                    }
                }
            }
            // Reset interrupt flag
            TC3->COUNT8.INTFLAG.bit.OVF = 1;
        }
    }

#ifdef __cplusplus
}
#endif

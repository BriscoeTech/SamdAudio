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

#ifndef SAMDAUDIO_H
#define SAMDAUDIO_H

	#include "Arduino.h"
	#include "Print.h"

	#include <SPI.h>


	//**********************************

	// defines the max number of channels that this device can handle
	#define MAX_N_CHANNELS		  4

	#define MAX_AUDIO_BUFFER_SIZE 1024 // value can be set in the sketch AUIDO_BUFFER_SIZE

	#define RAMPIN              255

	//**********************************

	class SamdAudio{
	public:

		SamdAudio(){};
		int begin(uint32_t sampleRate, uint8_t numOfChannels, uint16_t audio_buffer_size);
		void play(const char *fname, uint8_t channel) ;
		//void play(const char *fname) ;
		void stopChannel(uint8_t c);
		void end();

		void criticalON();
		void criticalOFF();

	private:

		void dacConfigure(void);

		//The first timer is used to feed the DAC with data every 1/sampleRate sec
		void configurePlayerTimer(uint32_t sampleRate);
		bool syncPlayerTimer(void);
		void resetPlayerTimer(void);
		void enablePlayerTimer(void);
		void disablePlayerTimer(void);

		//the second timer is used to read audio data from the SD card every 15.6ms
		void configureReaderTimer();
		void enableReaderTimer();
		void disableReaderTimer();

		void updaterConfigure();

		bool alonePlaying(uint8_t channel);
		bool someonePlaying();


	};
#endif //SAMDAUDIO_H

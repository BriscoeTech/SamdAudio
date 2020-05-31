/*

  Audio player, non blocking.
  read 8bit mono .wav file, up to 4 channels
  use Audacity to convert your audio file
  Author : AloyseTech

  03/17/19: https://github.com/hydronics2/SamdAudio
    updated to work with Adafruit Qaud Flash memory boards.
    This will work with any SAMD21 chip using SPI flash with a little tinkering
     - itsyBitsy M0 Express, 
     - Feather M0 Express, 
     - probalby M4 of abovev and trinkets 

    Read this great description in the Adafruit tutorial for getting the WAV files onto your Adafruit M0 Express board
    https://learn.adafruit.com/introducing-itsy-bitsy-m0?view=all#using-spi-flash
    Thanks to Tondy Dicola and Adafruit for making this so easy!

  if the sketch compiles and Initializes Audio player, but no sound, look in SamdAudio.cpp and umcomment these! 
   - #include <Adafruit_SPIFlash.h>
   - #include <Adafruit_SPIFlash_FatFs.h>
   - extern Adafruit_W25Q16BV_FatFs memory;
      
*/

#include <SPI.h>

#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>

#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
                                              // If you change this be
                                              // sure to change the fatfs
                                              // object type below to match.

#define FLASH_SS       SS1                    // Flash chip SS pin.
#define FLASH_SPI_PORT SPI1                   // What SPI port is Flash on?

Adafruit_SPIFlash flash(FLASH_SS, &FLASH_SPI_PORT);     // Use hardware SPI

// Finally create an Adafruit_M0_Express_CircuitPython object which gives
// an SD card-like interface to interacting with files stored in CircuitPython's
// flash filesystem.
//Adafruit_M0_Express_CircuitPython pythonfs(flash);
Adafruit_W25Q16BV_FatFs memory(flash);

#include <SamdAudio.h> // 
SamdAudio AudioPlayer; 

#define NUM_AUDIO_CHANNELS 4 //could be 1,2 or 4 for sound

#define AUDIO_BUFFER_SIZE 1024 //512 works fine for 22.05kh, use 1024 for 32khz and 44.1khz

//indicate sample rate here (use audacity to convert your wav)
const unsigned int sampleRate = 32000; //hz

//your wav file
const char *filename0 = "poem8bit_32khz.wav";
const char *filename1 = "sfx1_8bit_32khz.wav";
const char *filename2 = "sfx2_8bit_32khz.wav";
const char *filename3 = "sfx3_8bit_32khz.wav";
const char *filename4 = "sfx4_8bit_32khz.wav";

void setup()
{
  delay(10);
  Serial.begin(115200);
  while (!Serial); // open the serial to start!

  // Initialize flash library and check its chip ID.
  if (!flash.begin(FLASH_TYPE)) 
  {
    Serial.println("Error, failed to initialize flash chip!");
    while(1);
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.GetJEDECID(), HEX);
  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!memory.begin()) 
  {
    Serial.println("Error, failed to mount filesystem!");
    while(1);
  }
  Serial.println("Mounted filesystem!");  
  
  Serial.print("Initializing Audio Player...");
  if (AudioPlayer.begin(sampleRate, NUM_AUDIO_CHANNELS, AUDIO_BUFFER_SIZE) == -1) 
  {
    Serial.println(" failed!");
    return;
  }
  Serial.println(" done.");

  AudioPlayer.play(filename0, 1);

  Serial.println("Playing file.....");
}


void loop()
{
  if (Serial.available()) 
  {
    char c = Serial.read();
    Serial.println(c); //for debug

    if ( c == 'o') 
    {
      AudioPlayer.play(filename1, 0); //playing file on channel 0
      Serial.println("playing audio file on channel 0");
    }
    if ( c == 'p') 
    {
      AudioPlayer.play(filename2, 1); //playing file  on channel 1
      Serial.println("playing audio file on channel 1");
    }
      if ( c == 'k') 
    {
      AudioPlayer.play(filename3, 2); //playing file on channel 2
      Serial.println("playing audio file on channel 2");
    }
      if ( c == 'l') 
    {
      AudioPlayer.play(filename4, 3); //playing file on channel 3
      Serial.println("playing audio file on channel 3");
    }    
  } 
}


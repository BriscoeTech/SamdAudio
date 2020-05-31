/*
  Audio player, non blocking.
  read 8bit mono .wav file, up to 4 channels
  use Audacity to convert your audio file

  Author : AloyseTech
  Modified: BriscoeTech
  03/17/19: https://github.com/hydronics2/SamdAudio


  if the sketch compiles, but no sound, check to make sure:
   - '#include <SdFat.h>' is uncommented in SamdAudio.cpp 
   - 'extern SdFat memory;' is ucommented in SamdAudio.cpp

  
*/

#include <SdFat.h>  //https://github.com/greiman/SdFat
SdFat memory;

#include <SamdAudio.h>

SamdAudio AudioPlayer;

#define NUM_AUDIO_CHANNELS 4 //could be 1,2 or 4 for sound

#define AUDIO_BUFFER_SIZE 512 //512 works fine for 22.05kh, 
//set buffer size to 1024 for 32khz and 44.1khz for Adafruit M0 with QUAD Flash

//indicate sample rate here (use audacity to convert your wav)
const unsigned int sampleRate = 22050; //hz

//your wav file
const char *filename0 = "piano8bit_22khz.wav";
const char *filename1 = "sfx1_8bit_22khz.wav";
const char *filename2 = "sfx2_8bit_22khz.wav";
const char *filename3 = "sfx3_8bit_22khz.wav";
const char *filename4 = "sfx4_8bit_22khz.wav";

// SD chip select pin (with ethernet shield : 4)
#define YOUR_SD_CS 10

void setup()
{
  pinMode(YOUR_SD_CS, OUTPUT);


if ( !memory.begin(YOUR_SD_CS) )
  {
     //the sd card error was found, exit this function and to not attempt to finish
     Serial.println("can't start card");
  }

  Serial.begin(115200);
  while (!Serial); // open the serial to start!

  Serial.print("Initializing Audio Player...");
  if (AudioPlayer.begin(sampleRate, NUM_AUDIO_CHANNELS, AUDIO_BUFFER_SIZE) == -1) 
  {
    Serial.println(" failed!");
    return;
  }
  Serial.println(" done.");

  AudioPlayer.play(filename0, 1); //play 1st file on channel 0

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

// Rev: 08-19-2021 by Randy: Write last-song-number-started in EEPROM so we'll start with next song next time.
//      Was not able to get write to SD Card working properly so just use EEPROM.  This works perfectly.
//      This is the code that is used in the ELEVATOR as of 8/20/21.  Also I plugged the Arduino/sound board into
//      the same switched jack that the powered speakers are plugged into -- so they both turn OFF when elevator
//      power goes off, instead of only the amp turning off and the sound board running 24x7.
// Rev: 01-29-2019 by Randy (original version, updated 8/19/21)
// SD_Card_Audio will play songs on an Arduino using an SD card.  Similar to using a WAV Trigger board.
// 1. Convert music to WAV files per specs below, using link below.
// 2. Copy music onto an SD card (8.3-style filenames).
//    IMPORTANT: Filenames must be "##.wav" i.e. "03.wav" (and not "3.wav")
// 3. Store the number of song as an ASCII string i.e. "12" in the count.txt file on the SD card (write with Notepad app)

// From this article (similar to many): https://www.instructables.com/id/Audio-Player-Using-Arduino-With-Micro-SD-Card/
// Utilizes standard Arduino SD library, SD card and output device (Speaker, Headphones, Amplifier, etc)
// WAV files, 8-bit, 8-32khz Sample Rate, mono.
// Supported devices: Arduino Uno, Nano, Mega, etc.
// Documentation is available on the Wiki: https://github.com/TMRh20/TMRpcm/wiki
// See the wiki https://github.com/TMRh20/TMRpcm/wiki/Advanced-Features  
// Convert audio files here: https://audio.online-convert.com/convert-to-wav or use Audacity.
//   8 bit, 16000 Hz (or 32000, 22050, or 11025), mono, PCM unsigned 8-bit
// In Audacity:
//   Tracks, Mix, Mix Stereo down to mono
//   Tracks, Resample, Sample Rate 16000
//   IMPORTANT: "Project Rate" at bottom left of Audacity window must be set to 16000 before saving (even though resampled to 16000)
//   File, Export, Export Audio.  "Other uncompressed files", WAV, Unsigned 8-bit PCM.

// Mega Ground to speaker out common (to headphone jack Sleeve)
// Mega pin 46 to speaker out hot (to headphone jack Tip and Ring for mono out to stereo speakers)
// Mega pins 50-53, 5v, and Ground to SD card adapter
//   Pin 50 to MISO
//   Pin 51 to MOSI
//   Pin 52 to SCK
//   Pin 53 to SS/CS
// For testing, ground NEXT_TRACK_PIN to skip to the next track before current song ends.
// Note that volume level 5 is max without distortion.

#include <EEPROM.h>  // 8/21: Used to write the number of the song most recently started
#include <pcmRF.h>
#include <pcmConfig.h>
#include <SD.h>                      // need to include the SD library
#define SD_ChipSelectPin 53  //hardware SS pin 53 on Mega2560.
#include <TMRpcm.h>
#include <SPI.h>

TMRpcm audio;

File myFile;  // Create an SD card file-type object called myFile, which will hold a count of the number of songs on the SD card
int numSongs = 0;  // Will hold the count from count.txt file on SD card, i.e. 5 songs would be "5"
const byte NEXT_TRACK_PIN = 2;  // Ground this pin to skip to the next track

// **************************************************************************

// DEFINE THE MAXIMUM NUMBER AND NAMES OF ALL SONGS TO BE PLAYED ON THE SD CARD HERE:
const int MAXSONGS = 50;  // If we want more than 99, we'll need to go to a longer filename, or less clumsy code
char filename[MAXSONGS][13] = 
  {
    "01.wav", "02.wav", "03.wav", "04.wav", "05.wav", "06.wav", "07.wav", "08.wav", "09.wav", "10.wav",
    "11.wav", "12.wav", "13.wav", "14.wav", "15.wav", "16.wav", "17.wav", "18.wav", "19.wav", "20.wav",
    "21.wav", "22.wav", "23.wav", "24.wav", "25.wav", "26.wav", "27.wav", "28.wav", "29.wav", "30.wav",
    "31.wav", "32.wav", "33.wav", "34.wav", "35.wav", "36.wav", "37.wav", "38.wav", "39.wav", "40.wav",
    "41.wav", "42.wav", "43.wav", "44.wav", "45.wav", "46.wav", "47.wav", "48.wav", "49.wav", "50.wav"
  };

// **************************************************************************

void setup() {

  Serial.begin(9600);
  if(!SD.begin(SD_ChipSelectPin)) {
    Serial.println("SD fail");
    return;
  }

  // Figure out how many songs are on this card.  Will be read as an ASCII number i.e. "3", "15", etc.
  myFile = SD.open("count.txt");
  if (myFile) {
    numSongs = myFile.parseInt();  // Read ASCII chars in count.txt and convert to an integer.  One song would be "1" <CR/LF>
    myFile.close();
  } else {
    Serial.println("Error opening count.txt");
    while (true) { }
  }
  Serial.print("Number of songs: "); Serial.println(numSongs);

  // EEPROM address 0 holds number of the song last started, i.e. 7 for the 7th song (not offset zero.)

  audio.speakerPin = 46;     // set to 5,6,11 or 46 for Mega, 9 for Uno, Nano, etc.
  audio.disable();           // disables the timer on output pin and stops the music
  audio.stopPlayback();      // stops the music, but leaves the timer running
  audio.isPlaying();         // returns 1 if music playing, 0 if not
  audio.pause();             // pauses/unpauses playback
  audio.quality(1);          // 0 or 1. Set 1 for 2x oversampling
  audio.volume(0);           // 1(up) or 0(down) to control volume
  audio.setVolume(5);        // 0 to 7. Set volume level
  audio.loop(0);             // 0 or 1. Can be changed during playback for full control of looping. 
  //audio.play("filename");    // plays a file
  //audio.play("filename",30); // plays a file starting at 30 seconds into the track
  pinMode(NEXT_TRACK_PIN, INPUT_PULLUP);  // Ground this pin during playback to skip to next track (for testing purposes)
}

void loop() {

  int s = 0;  // Will hold the filename[] array index number of song to play.  I.e s = 0 for song no. 1 ("01.wav")
  
  while (true) {
    // Get the number of the last song started, and increment MOD numSongs
    s = EEPROM.read(0) - 1;  // s will now be in the range 0..numSongs - 1)
    // If this is the first time, EEPROM may contain garbage so initialize to first song...
    if ((s < 0) || (s > (numSongs - 1))) {  // Should only happen when we first use an Arduino
      s = 0;
    }
    Serial.print("Last song started was song number: "); Serial.println(s + 1);
    s = ((s + 1) % numSongs);  // I.e. 0..numSongs - 1

    // Now write (s + 1) (since not zero offset) to EEPROM, as the last song that was started.
    Serial.print("Writing started song offset: "); Serial.println(s + 1);  // Song number i.e. 1..50, not offset 0..49
    EEPROM.write(0, s + 1);  // i.e. 1..50, not 0..49

    Serial.print("So we will now play song number: "); Serial.println(s + 1);
    audio.setVolume(0);       // Avoids a loud click when starts playing
    audio.play(filename[s]);
    delay(100);               // Avoids click
    audio.setVolume(3);  // Can go as high as volume 5
    while ((audio.isPlaying()) && (digitalRead(NEXT_TRACK_PIN) != LOW)) { }  // Wait until song ends, or test pin gets grounded
    audio.stopPlayback();
    Serial.println("Song ended.");
    delay(1000);
  }
}

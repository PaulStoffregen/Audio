#include <SD.h>
#include <SPI.h>

File root;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20
const int chipSelect = 10;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  SPI.setMOSI(7);

  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  pinMode(10, OUTPUT);

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  root = SD.open("/");
  
  printDirectory(root, 0);
  
  Serial.println("done!");
  
  pinMode(2, OUTPUT);
  File f1 = SD.open("01_16S.WAV");
  File f2 = SD.open("01_16M.WAV");
  if (f1 && f2) {
    Serial.println("reading file");
    char buffer[512];
    int n, sum1=0, sum2=0;
    elapsedMillis ms;
    //while(f2.available()) {
    //f1.read(buffer, 44);
    //f2.read(buffer, 44);
    int size = 512;
    while (1) {
      digitalWriteFast(2, HIGH);
      n = f1.read(buffer, size);
      sum1 += n;
      digitalWriteFast(2, LOW);
      n = f2.read(buffer, size);
      sum2 += n;
      if (n < size || sum1 > 2000000) break;
    }
    float sec = (float)ms / 1000.0;
    Serial.print("seconds = ");
    Serial.println(sec);
    Serial.print("bytes per second = ");
    Serial.println((float)sum1 / sec);
    Serial.print("bytes per second = ");
    Serial.println((float)sum2 / sec);
  }
  
  
}

void loop()
{
  // nothing happens after setup finishes.
}

void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       //printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}




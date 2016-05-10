
#define NOTE_E2 82.41
#define NOTE_F2 87.31
#define NOTE_G2 98.00
#define NOTE_A2 110.0
#define NOTE_B2 123.5
#define NOTE_C3 130.8
#define NOTE_D3 146.8
#define NOTE_E3 164.8
#define NOTE_F3 174.6
#define NOTE_G3 196.0
#define NOTE_A3 220.0
#define NOTE_B3 246.9
#define NOTE_C4 261.6
#define NOTE_D4 293.7
#define NOTE_E4 329.6
#define NOTE_F4 349.2
#define NOTE_G4 392.0
#define NOTE_A4 440.0
#define NOTE_B4 493.9

// according to http://www.guitar-chords.org.uk/
              // open =  NOTE_E2  NOTE_A2  NOTE_D3  NOTE_G3  NOTE_B3  NOTE_E4
const float Cmajor[6] = {      0, NOTE_C3, NOTE_E3, NOTE_G3, NOTE_C4, NOTE_E4};  // C - E - G
const float Gmajor[6] = {NOTE_G2, NOTE_B2, NOTE_D3, NOTE_G3, NOTE_B3, NOTE_E4};  // G - B - D
const float Aminor[6] = {      0, NOTE_A2, NOTE_E3, NOTE_A3, NOTE_C4, NOTE_E4};  // A - C - E
const float Eminor[6] = {NOTE_E2, NOTE_B2, NOTE_E3, NOTE_G3, NOTE_B3, NOTE_E4};  // E - G - B

//                   E2, F2, F2#, G2, G2#, A2, A2#, B2
// C3, C3#, D3, D3#, E3, F3, F3#, G3, G3#, A3, A3#, B3
// C4, C4#, D4, D4#, E4, F4, F4#, G4, G4#, A4, A4#, B4


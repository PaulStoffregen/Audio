/* Testing new synth module Rectifier
 *  https://forum.pjrc.com/threads/61384?p=243547&viewfull=1#post243547
 *
 *  Copyright 2020 Bradley Sanders. This is free software under GPL2.
 *  Based on Revalogics vocoder and incorporating new Rectifier module
 *  to generate dc control voltages
 *
 *  Teensy 4.0 or higher is required to run this example.  It uses
 *  about 31% of the CPU time on Teensy 4.0 when running at 600 MHz.
 */
#include <Audio.h>

const int myInput = AUDIO_INPUT_LINEIN;

// GUItool: begin automatically generated code
AudioInputI2S            i2s2;           //xy=138.22221755981445,1116.9999799728394
AudioMixer4              mixer1;         //xy=303,1125
AudioMixer4              mixer2;         //xy=303,1187
AudioFilterStateVariable filter39;       //xy=468,1259
AudioFilterStateVariable filter41;       //xy=469,1306
AudioFilterStateVariable filter43;       //xy=469,1351
AudioFilterStateVariable filter45;       //xy=469,1397
AudioFilterStateVariable filter47;       //xy=469,1443
AudioFilterStateVariable filter49;       //xy=469,1489
AudioFilterStateVariable filter51;       //xy=469,1535
AudioFilterStateVariable filter1;        //xy=476,727
AudioFilterStateVariable filter3;        //xy=476,773
AudioFilterStateVariable filter5;        //xy=476,818
AudioFilterStateVariable filter7;        //xy=476,864
AudioFilterStateVariable filter9;        //xy=476,910
AudioFilterStateVariable filter11;       //xy=476,956
AudioFilterStateVariable filter13;       //xy=476,1002
AudioFilterStateVariable filter40;       //xy=606,1260
AudioFilterStateVariable filter42;       //xy=608,1305
AudioFilterStateVariable filter44;       //xy=608,1350
AudioFilterStateVariable filter46;       //xy=608,1396
AudioFilterStateVariable filter48;       //xy=608,1442
AudioFilterStateVariable filter50;       //xy=608,1488
AudioFilterStateVariable filter52;       //xy=608,1534
AudioFilterStateVariable filter2;        //xy=615,726
AudioFilterStateVariable filter4;        //xy=615,772
AudioFilterStateVariable filter6;        //xy=615,817
AudioFilterStateVariable filter8;        //xy=615,863
AudioFilterStateVariable filter10;       //xy=615,909
AudioFilterStateVariable filter12;       //xy=615,955
AudioFilterStateVariable filter14;       //xy=615,1001
AudioAnalyzePeak         peak21;         //xy=622.8888740539551,1154.777837753296
AudioAnalyzePeak         peak20;         //xy=625.111083984375,1121.3332829475403
AudioEffectRectifier          Rect5; //xy=749,908
AudioEffectRectifier          Rect7; //xy=749,995
AudioEffectRectifier          Rect4; //xy=750,865
AudioEffectRectifier          Rect6; //xy=751,955
AudioEffectRectifier          Rect2; //xy=753,774
AudioEffectRectifier          Rect3; //xy=754,819
AudioEffectRectifier          Rect1;          //xy=755,725
AudioEffectMultiply      multiply2;      //xy=758,1257
AudioEffectMultiply      multiply3; //xy=758,1311
AudioEffectMultiply      multiply6; //xy=758,1447
AudioEffectMultiply      multiply7; //xy=758,1493
AudioEffectMultiply      multiply4; //xy=759,1358
AudioEffectMultiply      multiply8; //xy=759,1539
AudioEffectMultiply      multiply5; //xy=760,1402
AudioMixer4              mixer3; //xy=841,1638
AudioMixer4              mixer4;         //xy=842,1700
AudioFilterBiquad        biquad4; //xy=888,774
AudioFilterBiquad        biquad3;        //xy=889,724
AudioFilterBiquad        biquad6; //xy=889,863
AudioFilterBiquad        biquad9; //xy=889,995
AudioFilterBiquad        biquad7; //xy=890,909
AudioFilterBiquad        biquad8; //xy=890,955
AudioFilterBiquad        biquad5;   //xy=891,819
AudioFilterStateVariable filter53;       //xy=947,1260
AudioFilterStateVariable filter55;       //xy=947,1306
AudioFilterStateVariable filter57;       //xy=947,1351
AudioFilterStateVariable filter59;       //xy=947,1397
AudioFilterStateVariable filter61;       //xy=947,1443
AudioFilterStateVariable filter63;       //xy=947,1489
AudioFilterStateVariable filter65;       //xy=947,1535
AudioSynthNoiseWhite     noise1;         //xy=1079.222255706787,1143.2221994400024
AudioFilterStateVariable filter54;       //xy=1086,1259
AudioFilterStateVariable filter56;       //xy=1086,1305
AudioFilterStateVariable filter58;       //xy=1086,1350
AudioFilterStateVariable filter60;       //xy=1086,1396
AudioFilterStateVariable filter62;       //xy=1086,1442
AudioFilterStateVariable filter64;       //xy=1086,1488
AudioFilterStateVariable filter66;       //xy=1086,1534
AudioFilterStateVariable filter15;       //xy=1103,704
AudioFilterStateVariable filter17;       //xy=1103,750
AudioFilterStateVariable filter19;       //xy=1103,795
AudioFilterStateVariable filter21;       //xy=1103,841
AudioFilterStateVariable filter23;       //xy=1103,887
AudioFilterStateVariable filter25;       //xy=1103,933
AudioFilterStateVariable filter27;       //xy=1103,979
AudioEffectMultiply      multiply9; //xy=1226,1257
AudioEffectMultiply      multiply10; //xy=1227,1310
AudioEffectMultiply      multiply11; //xy=1227,1354
AudioEffectMultiply      multiply13; //xy=1228,1446
AudioEffectMultiply      multiply14; //xy=1229,1493
AudioEffectMultiply      multiply15; //xy=1229,1540
AudioEffectMultiply      multiply12; //xy=1231,1407
AudioFilterStateVariable filter16;       //xy=1242,703
AudioFilterStateVariable filter18;       //xy=1242,749
AudioFilterStateVariable filter20;       //xy=1242,794
AudioFilterStateVariable filter22;       //xy=1242,840
AudioFilterStateVariable filter24;       //xy=1242,886
AudioFilterStateVariable filter26;       //xy=1242,932
AudioFilterStateVariable filter28;       //xy=1242,978
AudioMixer4              mixer5;         //xy=1305,1640
AudioMixer4              mixer6;         //xy=1306,1702
AudioEffectRectifier          Rect9; //xy=1373,748
AudioEffectRectifier          Rect8; //xy=1374,704
AudioEffectRectifier          Rect11; //xy=1375,841
AudioEffectRectifier          Rect10; //xy=1379,795
AudioEffectRectifier          Rect12; //xy=1379,888
AudioEffectRectifier          Rect13; //xy=1379,932
AudioEffectRectifier          Rect14; //xy=1381,982
AudioFilterBiquad        biquad10; //xy=1505,706
AudioFilterBiquad        biquad13; //xy=1505,842
AudioFilterBiquad        biquad11; //xy=1506,748
AudioFilterBiquad        biquad12; //xy=1507,795
AudioFilterBiquad        biquad14; //xy=1508,888
AudioFilterBiquad        biquad15; //xy=1509,932
AudioFilterBiquad        biquad16; //xy=1511,980
AudioFilterStateVariable filter67;       //xy=1517,1246
AudioFilterStateVariable filter69;       //xy=1517,1292
AudioFilterStateVariable filter71;       //xy=1517,1337
AudioFilterStateVariable filter73;       //xy=1517,1383
AudioFilterStateVariable filter75;       //xy=1517,1429
AudioFilterStateVariable filter77;       //xy=1517,1475
AudioMixer4              mixer9;         //xy=1628.1111221313477,1624.6666507720947
AudioMixer4              mixer10;        //xy=1629,1700
AudioFilterStateVariable filter68;       //xy=1656,1245
AudioFilterStateVariable filter70;       //xy=1656,1291
AudioFilterStateVariable filter72;       //xy=1656,1336
AudioFilterStateVariable filter74;       //xy=1656,1382
AudioFilterStateVariable filter76;       //xy=1656,1428
AudioFilterStateVariable filter78;       //xy=1656,1474
AudioFilterStateVariable filter29;       //xy=1749,778
AudioFilterStateVariable filter31;       //xy=1749,824
AudioFilterStateVariable filter33;       //xy=1749,869
AudioFilterStateVariable filter35;       //xy=1749,915
AudioFilterStateVariable filter37;       //xy=1749,961
AudioMixer4              mixer11;        //xy=1791,1665
AudioEffectMultiply      multiply21; //xy=1798,1478
AudioEffectMultiply      multiply20; //xy=1799.222267150879,1427.999948501587
AudioEffectMultiply      multiply17; //xy=1800,1290
AudioEffectMultiply      multiply19; //xy=1800,1384
AudioEffectMultiply      multiply16;  //xy=1801,1242
AudioEffectMultiply      multiply18; //xy=1801,1338
AudioFilterStateVariable filter30;       //xy=1888,777
AudioFilterStateVariable filter32;       //xy=1888,823
AudioFilterStateVariable filter34;       //xy=1888,868
AudioFilterStateVariable filter36;       //xy=1888,914
AudioFilterStateVariable filter38;       //xy=1888,960
AudioMixer4              mixer7;         //xy=1981,1346
AudioMixer4              mixer8;         //xy=1980.888957977295,1405.6665725708008
AudioEffectRectifier          Rect17; //xy=2018,869
AudioEffectRectifier          Rect18;  //xy=2018,914
AudioEffectRectifier          Rect19; //xy=2020,961
AudioEffectRectifier          Rect16; //xy=2021,822
AudioEffectRectifier          Rect15; //xy=2022,777
AudioOutputI2S           i2s1;           //xy=2094.2222061157227,1631.8888421058655
AudioFilterBiquad        biquad18; //xy=2154,824
AudioFilterBiquad        biquad17; //xy=2158,780
AudioFilterBiquad        biquad19; //xy=2158,870
AudioFilterBiquad        biquad21; //xy=2158,962
AudioFilterBiquad        biquad20; //xy=2159,912
//AudioOutputUSB           usb1;           //xy=2187.555633544922,661.6666431427002
AudioConnection          patchCord1(i2s2, 1, mixer1, 0);
AudioConnection          patchCord2(i2s2, 0, mixer2, 0);
AudioConnection          patchCord3(mixer1, 0, filter1, 0);
AudioConnection          patchCord4(mixer1, 0, filter3, 0);
AudioConnection          patchCord5(mixer1, 0, filter5, 0);
AudioConnection          patchCord6(mixer1, 0, filter7, 0);
AudioConnection          patchCord7(mixer1, 0, filter9, 0);
AudioConnection          patchCord8(mixer1, 0, filter11, 0);
AudioConnection          patchCord9(mixer1, 0, filter13, 0);
AudioConnection          patchCord10(mixer1, 0, filter15, 0);
AudioConnection          patchCord11(mixer1, 0, filter17, 0);
AudioConnection          patchCord12(mixer1, 0, filter19, 0);
AudioConnection          patchCord13(mixer1, 0, filter21, 0);
AudioConnection          patchCord14(mixer1, 0, filter23, 0);
AudioConnection          patchCord15(mixer1, 0, filter25, 0);
AudioConnection          patchCord16(mixer1, 0, filter27, 0);
AudioConnection          patchCord17(mixer1, 0, filter29, 0);
AudioConnection          patchCord18(mixer1, 0, filter31, 0);
AudioConnection          patchCord19(mixer1, 0, filter33, 0);
AudioConnection          patchCord20(mixer1, 0, filter35, 0);
AudioConnection          patchCord21(mixer1, 0, filter37, 0);
AudioConnection          patchCord22(mixer1, peak20);
//AudioConnection          patchCord23(mixer1, 0, usb1, 0);
AudioConnection          patchCord24(mixer1, 0, i2s1, 0);
AudioConnection          patchCord25(mixer2, 0, filter39, 0);
AudioConnection          patchCord26(mixer2, 0, filter41, 0);
AudioConnection          patchCord27(mixer2, 0, filter43, 0);
AudioConnection          patchCord28(mixer2, 0, filter45, 0);
AudioConnection          patchCord29(mixer2, 0, filter47, 0);
AudioConnection          patchCord30(mixer2, 0, filter49, 0);
AudioConnection          patchCord31(mixer2, 0, filter51, 0);
AudioConnection          patchCord32(mixer2, 0, filter53, 0);
AudioConnection          patchCord33(mixer2, 0, filter55, 0);
AudioConnection          patchCord34(mixer2, 0, filter57, 0);
AudioConnection          patchCord35(mixer2, 0, filter59, 0);
AudioConnection          patchCord36(mixer2, 0, filter61, 0);
AudioConnection          patchCord37(mixer2, 0, filter63, 0);
AudioConnection          patchCord38(mixer2, 0, filter65, 0);
AudioConnection          patchCord39(mixer2, 0, filter67, 0);
AudioConnection          patchCord40(mixer2, 0, filter69, 0);
AudioConnection          patchCord41(mixer2, 0, filter71, 0);
AudioConnection          patchCord42(mixer2, 0, filter73, 0);
AudioConnection          patchCord43(mixer2, 0, filter75, 0);
AudioConnection          patchCord44(mixer2, 0, mixer11, 2);
AudioConnection          patchCord45(mixer2, peak21);
AudioConnection          patchCord46(filter39, 0, filter40, 0);
AudioConnection          patchCord47(filter41, 1, filter42, 0);
AudioConnection          patchCord48(filter43, 1, filter44, 0);
AudioConnection          patchCord49(filter45, 1, filter46, 0);
AudioConnection          patchCord50(filter47, 1, filter48, 0);
AudioConnection          patchCord51(filter49, 1, filter50, 0);
AudioConnection          patchCord52(filter51, 1, filter52, 0);
AudioConnection          patchCord53(filter1, 0, filter2, 0);
AudioConnection          patchCord54(filter3, 1, filter4, 0);
AudioConnection          patchCord55(filter5, 1, filter6, 0);
AudioConnection          patchCord56(filter7, 1, filter8, 0);
AudioConnection          patchCord57(filter9, 1, filter10, 0);
AudioConnection          patchCord58(filter11, 1, filter12, 0);
AudioConnection          patchCord59(filter13, 1, filter14, 0);
AudioConnection          patchCord60(filter40, 0, multiply2, 0);
AudioConnection          patchCord61(filter42, 1, multiply3, 0);
AudioConnection          patchCord62(filter44, 1, multiply4, 0);
AudioConnection          patchCord63(filter46, 1, multiply5, 0);
AudioConnection          patchCord64(filter48, 1, multiply6, 0);
AudioConnection          patchCord65(filter50, 1, multiply7, 0);
AudioConnection          patchCord66(filter52, 1, multiply8, 0);
AudioConnection          patchCord67(filter2, 0, Rect1, 0);
AudioConnection          patchCord68(filter4, 1, Rect2, 0);
AudioConnection          patchCord69(filter6, 1, Rect3, 0);
AudioConnection          patchCord70(filter8, 1, Rect4, 0);
AudioConnection          patchCord71(filter10, 1, Rect5, 0);
AudioConnection          patchCord72(filter12, 1, Rect6, 0);
AudioConnection          patchCord73(filter14, 1, Rect7, 0);
AudioConnection          patchCord74(Rect5, biquad7);
AudioConnection          patchCord75(Rect7, biquad9);
AudioConnection          patchCord76(Rect4, biquad6);
AudioConnection          patchCord77(Rect6, biquad8);
AudioConnection          patchCord78(Rect2, biquad4);
AudioConnection          patchCord79(Rect3, biquad5);
AudioConnection          patchCord80(Rect1, biquad3);
AudioConnection          patchCord81(multiply2, 0, mixer3, 0);
AudioConnection          patchCord82(multiply3, 0, mixer3, 1);
AudioConnection          patchCord83(multiply6, 0, mixer4, 0);
AudioConnection          patchCord84(multiply7, 0, mixer4, 1);
AudioConnection          patchCord85(multiply4, 0, mixer3, 2);
AudioConnection          patchCord86(multiply8, 0, mixer4, 2);
AudioConnection          patchCord87(multiply5, 0, mixer3, 3);
AudioConnection          patchCord88(mixer3, 0, mixer9, 0);
AudioConnection          patchCord89(mixer4, 0, mixer9, 1);
AudioConnection          patchCord90(biquad4, 0, multiply3, 1);
AudioConnection          patchCord91(biquad3, 0, multiply2, 1);
AudioConnection          patchCord92(biquad6, 0, multiply5, 1);
AudioConnection          patchCord93(biquad9, 0, multiply8, 1);
AudioConnection          patchCord94(biquad7, 0, multiply6, 1);
AudioConnection          patchCord95(biquad8, 0, multiply7, 1);
AudioConnection          patchCord96(biquad5, 0, multiply4, 1);
AudioConnection          patchCord97(filter53, 1, filter54, 0);
AudioConnection          patchCord98(filter55, 1, filter56, 0);
AudioConnection          patchCord99(filter57, 1, filter58, 0);
AudioConnection          patchCord100(filter59, 1, filter60, 0);
AudioConnection          patchCord101(filter61, 1, filter62, 0);
AudioConnection          patchCord102(filter63, 1, filter64, 0);
AudioConnection          patchCord103(filter65, 1, filter66, 0);
AudioConnection          patchCord104(noise1, 0, filter77, 0);
AudioConnection          patchCord105(noise1, 0, mixer2, 1);
AudioConnection          patchCord106(filter54, 1, multiply9, 0);
AudioConnection          patchCord107(filter56, 1, multiply10, 0);
AudioConnection          patchCord108(filter58, 1, multiply11, 0);
AudioConnection          patchCord109(filter60, 1, multiply12, 0);
AudioConnection          patchCord110(filter62, 1, multiply13, 0);
AudioConnection          patchCord111(filter64, 1, multiply14, 0);
AudioConnection          patchCord112(filter66, 1, multiply15, 0);
AudioConnection          patchCord113(filter15, 1, filter16, 0);
AudioConnection          patchCord114(filter17, 1, filter18, 0);
AudioConnection          patchCord115(filter19, 1, filter20, 0);
AudioConnection          patchCord116(filter21, 1, filter22, 0);
AudioConnection          patchCord117(filter23, 1, filter24, 0);
AudioConnection          patchCord118(filter25, 1, filter26, 0);
AudioConnection          patchCord119(filter27, 1, filter28, 0);
AudioConnection          patchCord120(multiply9, 0, mixer5, 0);
AudioConnection          patchCord121(multiply10, 0, mixer5, 1);
AudioConnection          patchCord122(multiply11, 0, mixer5, 2);
AudioConnection          patchCord123(multiply13, 0, mixer6, 0);
AudioConnection          patchCord124(multiply14, 0, mixer6, 1);
AudioConnection          patchCord125(multiply15, 0, mixer6, 2);
AudioConnection          patchCord126(multiply12, 0, mixer5, 3);
AudioConnection          patchCord127(filter16, 1, Rect8, 0);
AudioConnection          patchCord128(filter18, 1, Rect9, 0);
AudioConnection          patchCord129(filter20, 1, Rect10, 0);
AudioConnection          patchCord130(filter22, 1, Rect11, 0);
AudioConnection          patchCord131(filter24, 1, Rect12, 0);
AudioConnection          patchCord132(filter26, 1, Rect13, 0);
AudioConnection          patchCord133(filter28, 1, Rect14, 0);
AudioConnection          patchCord134(mixer5, 0, mixer9, 2);
AudioConnection          patchCord135(mixer6, 0, mixer9, 3);
AudioConnection          patchCord136(Rect9, biquad11);
AudioConnection          patchCord137(Rect8, biquad10);
AudioConnection          patchCord138(Rect11, biquad13);
AudioConnection          patchCord139(Rect10, biquad12);
AudioConnection          patchCord140(Rect12, biquad14);
AudioConnection          patchCord141(Rect13, biquad15);
AudioConnection          patchCord142(Rect14, biquad16);
AudioConnection          patchCord143(biquad10, 0, multiply9, 1);
AudioConnection          patchCord144(biquad13, 0, multiply12, 1);
AudioConnection          patchCord145(biquad11, 0, multiply10, 1);
AudioConnection          patchCord146(biquad12, 0, multiply11, 1);
AudioConnection          patchCord147(biquad14, 0, multiply13, 1);
AudioConnection          patchCord148(biquad15, 0, multiply14, 1);
AudioConnection          patchCord149(biquad16, 0, multiply15, 1);
AudioConnection          patchCord150(filter67, 1, filter68, 0);
AudioConnection          patchCord151(filter69, 1, filter70, 0);
AudioConnection          patchCord152(filter71, 1, filter72, 0);
AudioConnection          patchCord153(filter73, 1, filter74, 0);
AudioConnection          patchCord154(filter75, 2, filter76, 0);
AudioConnection          patchCord155(filter77, 2, filter78, 0);
AudioConnection          patchCord156(mixer9, 0, mixer11, 0);
AudioConnection          patchCord157(mixer10, 0, mixer11, 1);
AudioConnection          patchCord158(filter68, 1, multiply16, 0);
AudioConnection          patchCord159(filter70, 1, multiply17, 0);
AudioConnection          patchCord160(filter72, 1, multiply18, 0);
AudioConnection          patchCord161(filter74, 1, multiply19, 0);
AudioConnection          patchCord162(filter76, 1, multiply20, 0);
AudioConnection          patchCord163(filter78, 2, multiply21, 0);
AudioConnection          patchCord164(filter29, 1, filter30, 0);
AudioConnection          patchCord165(filter31, 1, filter32, 0);
AudioConnection          patchCord166(filter33, 1, filter34, 0);
AudioConnection          patchCord167(filter35, 1, filter36, 0);
AudioConnection          patchCord168(filter37, 2, filter38, 0);
AudioConnection          patchCord169(mixer11, 0, i2s1, 1);
//AudioConnection          patchCord170(mixer11, 0, usb1, 1);
AudioConnection          patchCord171(multiply21, 0, mixer8, 1);
AudioConnection          patchCord172(multiply20, 0, mixer8, 0);
AudioConnection          patchCord173(multiply17, 0, mixer7, 1);
AudioConnection          patchCord174(multiply19, 0, mixer7, 3);
AudioConnection          patchCord175(multiply16, 0, mixer7, 0);
AudioConnection          patchCord176(multiply18, 0, mixer7, 2);
AudioConnection          patchCord177(filter30, 1, Rect15, 0);
AudioConnection          patchCord178(filter32, 1, Rect16, 0);
AudioConnection          patchCord179(filter34, 1, Rect17, 0);
AudioConnection          patchCord180(filter36, 1, Rect18, 0);
AudioConnection          patchCord181(filter38, 1, Rect19, 0);
AudioConnection          patchCord182(mixer7, 0, mixer10, 0);
AudioConnection          patchCord183(mixer8, 0, mixer10, 1);
AudioConnection          patchCord184(Rect17, biquad19);
AudioConnection          patchCord185(Rect18, biquad20);
AudioConnection          patchCord186(Rect19, biquad21);
AudioConnection          patchCord187(Rect16, biquad18);
AudioConnection          patchCord188(Rect15, biquad17);
AudioConnection          patchCord189(biquad18, 0, multiply17, 1);
AudioConnection          patchCord190(biquad17, 0, multiply16, 1);
AudioConnection          patchCord191(biquad19, 0, multiply18, 1);
AudioConnection          patchCord192(biquad21, 0, multiply20, 1);
AudioConnection          patchCord193(biquad21, 0, multiply21, 1);
AudioConnection          patchCord194(biquad20, 0, multiply19, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=2226.777786254883,1222.6666231155396
// GUItool: end automatically generated code


//---------------------------------------------------------------------------------------
void setup() {
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.7);

  const float res = 5;              // this is used as resonance value of all state variable filters
                                    // modulators
  const float freq[37] = {          // filter frequency table, tuned to specified musical notes
    110.0000000,  // A2   freq[0]
    123.4708253,  // B2   freq[1]
    138.5913155,  // C#3  freq[2]
    155.5634919,  // D#3  freq[3]
    174.6141157,  // F3   freq[4]
    195.9977180,  // G3   freq[5]
    220.0000000,  // A3   freq[6]
    246.9416506,  // B3   freq[7]
    277.1826310,  // C#4  freq[8]
    311.1269837,  // D#4  freq[9]
    349.2282314,  // F4   freq[10]
    391.9954360,  // G4   freq[11]
    440.0000000,  // A4   freq[12]
    493.8833013,  // B4   freq[13]
    554.3652620,  // C#5  freq[14]
    622.2539674,  // D#5  freq[15]
    698.4564629,  // F5   freq[16]
    783.9908720,  // G5   freq[17]
    880.0000000,  // A5   freq[18]
    987.7666025,  // B5   freq[19]
    1108.730524,  // C#6  freq[20]
    1244.507935,  // D#6  freq[21]
    1396.912926,  // F6   freq[22]
    1567.981744,  // G6   freq[23]
    1760.000000,  // A6   freq[24]
    1975.533205,  // B6   freq[25]
    2217.461048,  // C#7  freq[26]
    2489.015870,  // D#7  freq[27]
    2793.825851,  // F7   freq[28]
    3135.963488,  // G7   freq[29]
    3520.000000,  // A7   freq[30]
    3951.066410,  // B7   freq[31]
    4434.922096,  // C#8  freq[32]
    4978.031740,  // D#8  freq[33]
    5587.651703,  // F8   freq[34]
    6271.926976,  // G8   freq[35]
    7040.000000   // A8   freq[36]
  };

  AudioMemory(64);                  // allocate some memory for audio library
  Serial.begin(115200);             // initialize serial communication
  noise1.amplitude(0.7);            // controls sibilance (hiss syllables)
  mixer1.gain(0, 1);                // I2S left input level
  mixer2.gain(0, 0.7);              // I2S right input level
  mixer2.gain(1, 0.7);              // noise right input level
  mixer11.gain(0, 0.7);             // vocoder output 1 level (low-mid freq)
  mixer11.gain(1, 0.7);             // vocoder output 2 level (high freq)
  mixer11.gain(2, 0);               // instrument to output mix level

  mixer3.gain(0,1);
  mixer3.gain(1,1);
  mixer3.gain(2,1);
  mixer3.gain(3,1);
  mixer4.gain(0,1);
  mixer4.gain(1,1);
  mixer4.gain(2,1);

  mixer5.gain(0,1);
  mixer5.gain(1,1);
  mixer5.gain(2,1);
  mixer5.gain(3,1);
  mixer6.gain(0,1);
  mixer6.gain(1,1);
  mixer6.gain(2,1);

  mixer7.gain(0,1);
  mixer7.gain(1,1);
  mixer7.gain(2,1);
  mixer7.gain(3,1);
  mixer8.gain(0,1);
  mixer8.gain(1,1);

  mixer9.gain(0,1);
  mixer9.gain(1,1);
  mixer9.gain(2,1);
  mixer9.gain(3,1);
  mixer10.gain(0,1);
  mixer10.gain(1,1);

  filter1.resonance(res);                                       // set the resonance of the filters
  filter2.resonance(res);
  filter3.resonance(res);
  filter4.resonance(res);
  filter5.resonance(res);
  filter6.resonance(res);
  filter7.resonance(res);
  filter8.resonance(res);
  filter9.resonance(res);
  filter10.resonance(res);
  filter11.resonance(res);
  filter12.resonance(res);
  filter13.resonance(res);
  filter14.resonance(res);
  filter15.resonance(res);
  filter16.resonance(res);
  filter17.resonance(res);
  filter18.resonance(res);
  filter19.resonance(res);
  filter20.resonance(res);
  filter21.resonance(res);
  filter22.resonance(res);
  filter23.resonance(res);
  filter24.resonance(res);
  filter25.resonance(res);
  filter26.resonance(res);
  filter27.resonance(res);
  filter28.resonance(res);
  filter29.resonance(res);
  filter30.resonance(res);
  filter31.resonance(res);
  filter32.resonance(res);
  filter33.resonance(res);
  filter34.resonance(res);
  filter35.resonance(res);
  filter36.resonance(res);
  filter37.resonance(res);
  filter38.resonance(res);
  filter39.resonance(res);
  filter40.resonance(res);
  filter41.resonance(res);
  filter42.resonance(res);
  filter43.resonance(res);
  filter44.resonance(res);
  filter45.resonance(res);
  filter46.resonance(res);
  filter47.resonance(res);
  filter48.resonance(res);
  filter49.resonance(res);
  filter50.resonance(res);
  filter51.resonance(res);
  filter52.resonance(res);
  filter53.resonance(res);
  filter54.resonance(res);
  filter55.resonance(res);
  filter56.resonance(res);
  filter57.resonance(res);
  filter58.resonance(res);
  filter59.resonance(res);
  filter60.resonance(res);
  filter61.resonance(res);
  filter62.resonance(res);
  filter63.resonance(res);
  filter64.resonance(res);
  filter65.resonance(res);
  filter66.resonance(res);
  filter67.resonance(res);
  filter68.resonance(res);
  filter69.resonance(res);
  filter70.resonance(res);
  filter71.resonance(res);
  filter72.resonance(res);
  filter73.resonance(res);
  filter74.resonance(res);
  filter75.resonance(res);
  filter76.resonance(res);
  filter77.resonance(res);
  filter78.resonance(res);

  filter1.frequency(freq[0]);       // set the frequencies of the filters
  filter2.frequency(freq[0]);       // frequencies are assigned to a pair of filters because they are cascaded
  filter3.frequency(freq[2]);       // two sets of filters are used: for voice signal analysis and instrument/synth filter
  filter4.frequency(freq[2]);
  filter5.frequency(freq[4]);
  filter6.frequency(freq[4]);
  filter7.frequency(freq[6]);
  filter8.frequency(freq[6]);
  filter9.frequency(freq[8]);
  filter10.frequency(freq[8]);
  filter11.frequency(freq[10]);
  filter12.frequency(freq[10]);
  filter13.frequency(freq[12]);
  filter14.frequency(freq[12]);
  filter15.frequency(freq[14]);
  filter16.frequency(freq[14]);
  filter17.frequency(freq[16]);
  filter18.frequency(freq[16]);
  filter19.frequency(freq[18]);
  filter20.frequency(freq[18]);
  filter21.frequency(freq[20]);
  filter22.frequency(freq[20]);
  filter23.frequency(freq[22]);
  filter24.frequency(freq[22]);
  filter25.frequency(freq[24]);
  filter26.frequency(freq[24]);
  filter27.frequency(freq[26]);
  filter28.frequency(freq[26]);
  filter29.frequency(freq[28]);
  filter30.frequency(freq[28]);
  filter31.frequency(freq[30]);
  filter32.frequency(freq[30]);
  filter33.frequency(freq[32]);
  filter34.frequency(freq[32]);
  filter35.frequency(freq[34]);
  filter36.frequency(freq[34]);
  filter37.frequency(freq[36]);
  filter38.frequency(freq[36]);
  filter39.frequency(freq[0]);
  filter40.frequency(freq[0]);
  filter41.frequency(freq[2]);
  filter42.frequency(freq[2]);
  filter43.frequency(freq[4]);
  filter44.frequency(freq[4]);
  filter45.frequency(freq[6]);
  filter46.frequency(freq[6]);
  filter47.frequency(freq[8]);
  filter48.frequency(freq[8]);
  filter49.frequency(freq[10]);
  filter50.frequency(freq[10]);
  filter51.frequency(freq[12]);
  filter52.frequency(freq[12]);
  filter53.frequency(freq[14]);
  filter54.frequency(freq[14]);
  filter55.frequency(freq[16]);
  filter56.frequency(freq[16]);
  filter57.frequency(freq[18]);
  filter58.frequency(freq[18]);
  filter59.frequency(freq[20]);
  filter60.frequency(freq[20]);
  filter61.frequency(freq[22]);
  filter62.frequency(freq[22]);
  filter63.frequency(freq[24]);
  filter64.frequency(freq[24]);
  filter65.frequency(freq[26]);
  filter66.frequency(freq[26]);
  filter67.frequency(freq[28]);
  filter68.frequency(freq[28]);
  filter69.frequency(freq[30]);
  filter70.frequency(freq[30]);
  filter71.frequency(freq[32]);
  filter72.frequency(freq[32]);
  filter73.frequency(freq[34]);
  filter74.frequency(freq[34]);
  filter75.frequency(freq[36]);
  filter76.frequency(freq[36]);
  filter77.frequency(freq[36]);     // last pair of filters are used for sibilance
  filter78.frequency(freq[36]);     // input is a white noise, instead of instrument/synth

  biquad3.setLowpass(0, 90, 0.53);
  biquad3.setLowpass(1, 90, 0.707);
  biquad3.setLowpass(2, 60, 0.53);
  biquad3.setLowpass(3, 80, 0.707);
  biquad4.setLowpass(0, 200, 0.53);
  biquad4.setLowpass(1, 200, 0.707);
  biquad4.setLowpass(2, 60, 0.53);
  biquad4.setLowpass(3, 160, 0.707);
  biquad5.setLowpass(0, 200, 0.53);
  biquad5.setLowpass(1, 200, 0.707);
  biquad5.setLowpass(2, 60, 0.53);
  biquad5.setLowpass(3, 160, 0.707);
  biquad6.setLowpass(0, 200, 0.53);
  biquad6.setLowpass(1, 200, 0.707);
  biquad6.setLowpass(2, 60, 0.53);
  biquad6.setLowpass(3, 160, 0.707);
  biquad7.setLowpass(0, 200, 0.53);
  biquad7.setLowpass(1, 200, 0.707);
  biquad7.setLowpass(2, 60, 0.53);
  biquad7.setLowpass(3, 160, 0.707);
  biquad8.setLowpass(0, 200, 0.53);
  biquad8.setLowpass(1, 200, 0.707);
  biquad8.setLowpass(2, 60, 0.53);
  biquad8.setLowpass(3, 160, 0.707);
  biquad9.setLowpass(0, 200, 0.53);
  biquad9.setLowpass(1, 200, 0.707);
  biquad9.setLowpass(2, 60, 0.53);
  biquad9.setLowpass(3, 160, 0.707);
  biquad10.setLowpass(0, 200, 0.53);
  biquad10.setLowpass(1, 200, 0.707);
  biquad10.setLowpass(2, 60, 0.53);
  biquad10.setLowpass(3, 160, 0.707);
  biquad11.setLowpass(0, 200, 0.53);
  biquad11.setLowpass(1, 200, 0.707);
  biquad11.setLowpass(2, 60, 0.53);
  biquad11.setLowpass(3, 160, 0.707);
  biquad12.setLowpass(0, 200, 0.53);
  biquad12.setLowpass(1, 200, 0.707);
  biquad12.setLowpass(2, 60, 0.53);
  biquad12.setLowpass(3, 160, 0.707);
  biquad13.setLowpass(0, 200, 0.53);
  biquad13.setLowpass(1, 200, 0.707);
  biquad13.setLowpass(2, 60, 0.53);
  biquad13.setLowpass(3, 160, 0.707);
  biquad14.setLowpass(0, 200, 0.53);
  biquad14.setLowpass(1, 200, 0.707);
  biquad14.setLowpass(2, 60, 0.53);
  biquad14.setLowpass(3, 160, 0.707);
  biquad15.setLowpass(0, 200, 0.53);
  biquad15.setLowpass(1, 200, 0.707);
  biquad15.setLowpass(2, 60, 0.53);
  biquad15.setLowpass(3, 160, 0.707);
  biquad16.setLowpass(0, 200, 0.53);
  biquad16.setLowpass(1, 200, 0.707);
  biquad16.setLowpass(2, 60, 0.53);
  biquad16.setLowpass(3, 160, 0.707);
  biquad17.setLowpass(0, 200, 0.53);
  biquad17.setLowpass(1, 200, 0.707);
  biquad17.setLowpass(2, 60, 0.53);
  biquad17.setLowpass(3, 160, 0.707);
  biquad18.setLowpass(0, 200, 0.53);
  biquad18.setLowpass(1, 200, 0.707);
  biquad18.setLowpass(2, 60, 0.53);
  biquad18.setLowpass(3, 160, 0.707);
  biquad19.setLowpass(0, 200, 0.53);
  biquad19.setLowpass(1, 200, 0.707);
  biquad19.setLowpass(2, 60, 0.53);
  biquad19.setLowpass(3, 160, 0.707);
  biquad20.setLowpass(0, 200, 0.53);
  biquad20.setLowpass(1, 200, 0.707);
  biquad20.setLowpass(2, 60, 0.53);
  biquad20.setLowpass(3, 160, 0.707);
  biquad21.setLowpass(0, 200, 0.53);
  biquad21.setLowpass(1, 200, 0.707);
  biquad21.setLowpass(2, 60, 0.53);
  biquad21.setLowpass(3, 160, 0.707);
}

void loop() {
  Serial.print(AudioProcessorUsage());
  Serial.print("/");
  Serial.print(AudioProcessorUsageMax());
  Serial.println("");
  AudioProcessorUsageMaxReset();
  delay(100);
}

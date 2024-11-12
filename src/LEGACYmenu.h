// this is just a continuation of the Hexboard code but it's very long
// so it's in its own header.


/*
  This section of the code handles the
  dot matrix screen and, most importantly,
  the menu system display and controls.

  The following library is used: documentation
  is also available here.
    https://github.com/Spirik/GEM
*/

/*
  Create menu page object of class GEMPage. 
  Menu page holds menu items (GEMItem) and represents menu level.
  Menu can have multiple menu pages (linked to each other) with multiple menu items each.
  
  GEMPage constructor creates each page with the associated label.
  GEMItem constructor can create many different sorts of menu items.
  The items here are navigation links.
  The first parameter is the item label.
  The second parameter is the destination page when that item is selected.
*/
GEMPage  menuPageMain("HexBoard MIDI Controller");
GEMPage  menuPageTuning("Tuning");
GEMItem  menuGotoTuning("Tuning", menuPageTuning);
GEMItem  menuTuningBack("<< Back", menuPageMain);
GEMPage  menuPageLayout("Layout");
GEMItem  menuGotoLayout("Layout", menuPageLayout); 
GEMItem  menuLayoutBack("<< Back", menuPageMain);
GEMPage  menuPageScales("Scales");
GEMItem  menuGotoScales("Scales", menuPageScales); 
GEMItem  menuScalesBack("<< Back", menuPageMain);
GEMPage  menuPageColors("Color options");
GEMItem  menuGotoColors("Color options", menuPageColors);
GEMItem  menuColorsBack("<< Back", menuPageMain);
GEMPage  menuPageSynth("Synth options");
GEMItem  menuGotoSynth("Synth options", menuPageSynth);
GEMItem  menuSynthBack("<< Back", menuPageMain);
GEMPage  menuPageControl("Control wheel");
GEMItem  menuGotoControl("Control wheel", menuPageControl);
GEMItem  menuControlBack("<< Back", menuPageMain);
GEMPage  menuPageAdvanced("Advanced");
GEMItem  menuGotoAdvanced("Advanced", menuPageAdvanced);
GEMItem  menuAdvancedBack("<< Back", menuPageMain);
GEMPage  menuPageReboot("Ready to flash firmware!");
/*
  We haven't written the code for some procedures,
  but the menu item needs to know the address
  of procedures it has to run when it's selected.
  So we forward-declare a placeholder for the
  procedure like this, so that the menu item
  can be built, and then later we will define
  this procedure in full.
*/
/*
  This GEMItem is meant to just be a read-only text label.
  To be honest I don't know how to get just a plain text line to show here other than this!
*/
GEMItem  menuItemVersion("v1.0.1", fakeButton);
SelectOptionByte optionByteHardware[] =  {
  { "V1.1", HARDWARE_UNKNOWN }, { "V1.1" , HARDWARE_V1_1 },
  { "V1.2", HARDWARE_V1_2 }
};
GEMSelect selectHardware( sizeof(optionByteHardware)  / sizeof(SelectOptionByte), optionByteHardware);
GEMItem  menuItemHardware("HexBoard", Hardware_Version, selectHardware, GEM_READONLY);
/*
  This GEMItem runs a given procedure when you select it.
  We must declare or define that procedure first.
*/
GEMItem  menuItemUSBBootloader("Update Firmware", rebootToBootloader);
/*
  Tunings, layouts, scales, and keys are defined
  earlier in this code. We should not have to
  manually type in menu objects for those
  pre-loaded values. Instead, we will use routines to
  construct menu items automatically.
  
  These lines are forward declarations for
  the menu objects we will make later.
  This allocates space in memory with
  enough size to procedurally fill
  the objects based on the contents of
  the pre-loaded tuning/layout/etc. definitions
  we defined above.
*/
GEMItem* menuItemTuning[TUNINGCOUNT];       
GEMItem* menuItemLayout[layoutCount];  
GEMItem* menuItemScales[scaleCount];       
GEMSelect* selectKey[TUNINGCOUNT];         
GEMItem* menuItemKeys[TUNINGCOUNT];       
/*
  We are now creating some GEMItems that let you
  1) select a value from a list of options,
  2) update a given variable based on what was chosen,
  3) if necessary, run a procedure as well once the value's chosen.
  
  The list of options is in the form of a 2-d array.
  There are A arrays, one for each option.
  Each is 2 entries long. First entry is the label
  for that choice, second entry is the value associated.
  
  These arrays go into a typedef that depends on the type of the variable
  being selected (i.e. Byte for small positive integers; Int for
  sign-dependent and large integers).
  
  Then that typeDef goes into a GEMSelect object, with parameters
  equal to the number of entries in the array, and the storage size of one element
  in the array. The GEMSelect object is basically just a pointer to the
  array of choices. The GEMItem then takes the GEMSelect pointer as a parameter.
  
  The fact that GEM expects pointers and references makes it tricky
  to work with if you are new to C++.
*/
SelectOptionByte optionByteMPEpitchBend[] = { { "2", 2}, {"12", 12}, {"24", 24}, {"48", 48}, {"96", 96} };
GEMSelect selectMPEpitchBend( sizeof(optionByteMPEpitchBend) / sizeof(SelectOptionByte), optionByteMPEpitchBend);
GEMItem menuItemMPEpitchBend( "MPE Bend Range:", MPEpitchBendSemis, selectMPEpitchBend, assignPitches);

SelectOptionByte optionByteYesOrNo[] =  { { "No", 0 }, { "Yes" , 1 } };
GEMSelect selectYesOrNo( sizeof(optionByteYesOrNo)  / sizeof(SelectOptionByte), optionByteYesOrNo);
GEMItem  menuItemScaleLock( "Scale lock?", scaleLock, selectYesOrNo);
GEMItem  menuItemPercep( "Fix color:", perceptual, selectYesOrNo, setLEDcolorCodes);
GEMItem  menuItemShiftColor( "ColorByKey", paletteBeginsAtKeyCenter, selectYesOrNo, setLEDcolorCodes);
GEMItem  menuItemWheelAlt( "Alt wheel?", wheelMode, selectYesOrNo);

GEMItem  menuItemRotary( "Invert Encoder:", rotaryInvert);

SelectOptionByte optionByteWheelType[] = { { "Springy", 0 }, { "Sticky", 1} };
GEMSelect selectWheelType( sizeof(optionByteWheelType) / sizeof(SelectOptionByte), optionByteWheelType);
GEMItem  menuItemPBBehave( "Pitch bend", pbSticky, selectWheelType);
GEMItem  menuItemModBehave( "Mod wheel", modSticky, selectWheelType);

SelectOptionByte optionBytePlayback[] = { { "Off", SYNTH_OFF }, { "Mono", SYNTH_MONO }, { "Arp'gio", SYNTH_ARPEGGIO }, { "Poly", SYNTH_POLY } };
GEMSelect selectPlayback(sizeof(optionBytePlayback) / sizeof(SelectOptionByte), optionBytePlayback);
GEMItem  menuItemPlayback(  "Synth mode:",       playbackMode,  selectPlayback, resetSynthFreqs);

// Hardware V1.2-only
SelectOptionByte optionByteAudioD[] =  {
  { "Buzzer", AUDIO_PIEZO }, { "Jack" , AUDIO_AJACK }, { "Both", AUDIO_BOTH }
};
GEMSelect selectAudioD( sizeof(optionByteAudioD)  / sizeof(SelectOptionByte), optionByteAudioD);
GEMItem  menuItemAudioD("SynthOutput:", audioD, selectAudioD);

// Roland MT-32 mode (1987)
SelectOptionByte optionByteRolandMT32[] = {
  // Piano
  {"APiano1",  1}, {"APiano2",  2}, {"APiano3",  3},
  {"EPiano1",  4}, {"EPiano2",  5}, {"EPiano3",  6}, {"EPiano4",  7},
  {"HonkyTonk",8},
  // Organ
  {"EOrgan1",  9}, {"EOrgan2", 10}, {"EOrgan3", 11}, {"EOrgan4", 12},
  {"POrgan2", 13}, {"POrgan3", 14}, {"POrgan4", 15},
  {"Accordion",16},
  // Keybrd
  {"Harpsi1", 17}, {"Harpsi2", 18}, {"Harpsi3", 19},
  {"Clavi 1", 20}, {"Clavi 2", 21}, {"Clavi 3", 22},
  {"Celesta", 23}, {"Celest2", 24},
  // S Brass
  {"SBrass1", 25}, {"SBrass2", 26}, {"SBrass3", 27}, {"SBrass4", 28},
  // SynBass
  {"SynBass", 29}, {"SynBas2", 30}, {"SynBas3", 31}, {"SynBas4", 32},
  // Synth 1
  {"Fantasy", 33}, {"HarmoPan",34}, {"Chorale", 35}, {"Glasses", 36},
  {"Soundtrack",37},{"Atmosphere",38},{"WarmBell",39},{"FunnyVox",40},
  // Synth 2
  {"EchoBell",41}, {"IceRain", 42}, {"Oboe2K1", 43}, {"EchoPan", 44},
  {"Dr.Solo", 45}, {"SchoolDaze",46},{"BellSinger",47},{"SquareWave",48},
  // Strings
  {"StrSec1", 49}, {"StrSec2", 50}, {"StrSec3", 51}, {"Pizzicato", 52},
  {"Violin1", 53}, {"Violin2", 54}, {"Cello 1", 55}, {"Cello 2", 56},
  {"ContraBass",57}, {"Harp  1", 58}, {"Harp  2", 59},
  // Guitar
  {"Guitar1", 60}, {"Guitar2", 61}, {"EGuitr1", 62}, {"EGuitr2", 63},
  {"Sitar", 64},
  // Bass
  {"ABass 1", 65}, {"ABass 2", 66}, {"EBass 1", 67}, {"EBass 2", 68},
  {"SlapBass", 69},{"SlapBa2", 70}, {"Fretless", 71},{"Fretle2", 72},
  // Wind
  {"Flute 1", 73}, {"Flute 2", 74}, {"Piccolo", 75}, {"Piccol2", 76},
  {"Recorder",77}, {"PanPipes",78},
  {"Sax   1", 79}, {"Sax   2", 80}, {"Sax   3", 81}, {"Sax   4", 82},
  {"Clarinet",83}, {"Clarin2", 84}, {"Oboe",    85}, {"EnglHorn", 86},
  {"Bassoon", 87}, {"Harmonica",88},
  // Brass
  {"Trumpet", 89}, {"Trumpe2", 90}, {"Trombone",91}, {"Trombo2", 92},
  {"FrHorn1", 93}, {"FrHorn2", 94},
  {"Tuba", 95},    {"BrsSect", 96}, {"BrsSec2", 97},
  // Mallet
  {"Vibe  1", 98}, {"Vibe  2", 99},
  {"SynMallet",100}, {"WindBell",101}, {"Glock",102}, {"TubeBell",103}, {"XyloPhone",104}, {"Marimba",105},
  // Special
  {"Koto", 106}, {"Sho", 107}, {"Shakuhachi",108},
  {"Whistle",109}, {"Whistl2",110}, {"BottleBlow",111},{"BreathPipe",112},
  // Percussion
  {"Timpani",113}, {"MelTom", 114}, {"DeepSnare",115},
  {"ElPerc1",116}, {"ElPerc2",117}, {"Taiko",  118}, {"TaikoRim",119},
  {"Cymbal",120}, {"Castanets",121}, {"Triangle",122},
  // Effects
  {"OrchHit",123}, {"Telephone",124}, {"BirdTweet",125}, {"1NoteJam",126}, {"WaterBells",127}, {"JungleTune",128},
};
GEMSelect selectRolandMT32(sizeof(optionByteRolandMT32) / sizeof(SelectOptionByte), optionByteRolandMT32);
GEMItem  menuItemRolandMT32("RolandMT32:", programChange,  selectRolandMT32, sendProgramChange);

// General MIDI 1
SelectOptionByte optionByteGeneralMidi[] = {
  // Piano
  {"Piano 1", 1}, {"Piano 2", 2}, {"Piano 3", 3}, {"HonkyTonk", 4},
  {"EPiano1", 5}, {"EPiano2", 6}, {"HarpsiChord", 7}, {"Clavinet", 8},
  // Chromatic Percussion
  {"Celesta", 9},  {"Glockenspiel", 10}, {"MusicBox", 11}, {"Vibraphone", 12},
  {"Marimba", 13}, {"Xylophone", 14}, {"TubeBells", 15}, {"Dulcimer", 16},
  // Organ
  {"Organ 1", 17}, {"Organ 2", 18}, {"Organ 3", 19}, {"ChurchOrgan", 20},
  {"ReedOrgan", 21}, {"Accordion", 22}, {"Harmonica", 23}, {"Bandoneon", 24},
  // Guitar
  {"AGtrNylon", 25}, {"AGtrSteel", 26},
  {"EGtrJazz", 27}, {"EGtrClean", 28}, {"EGtrMuted", 29},
  {"EGtrOverdrive", 30}, {"EGtrDistortion", 31}, {"EGtrHarmonics", 32},
  // Bass
  {"ABass", 33}, {"EBasFinger", 34}, {"EBasPicked", 35}, {"EBasFretless", 36},
  {"SlpBass1", 37}, {"SlpBas2", 38}, {"SynBas1", 39}, {"SynBas2", 40},
  // Strings
  {"Violin", 41}, {"Viola", 42}, {"Cello", 43}, {"ContraBass", 44},
  {"TremoloStrings", 45}, {"PizzicatoStrings", 46}, {"OrchHarp", 47}, {"Timpani", 48},
  // Ensemble
  {"StrEns1", 49}, {"StrEns2", 50}, {"SynStr1", 51}, {"SynStr2", 52},
  {"ChoirAahs", 53}, {"VoiceOohs", 54}, {"SynVoice", 55}, {"OrchHit", 56},
  // Brass
  {"Trumpet", 57}, {"Trombone", 58}, {"Tuba", 59}, {"MutedTrumpet", 60},
  {"FrenchHorn", 61}, {"BrassSection", 62}, {"SynBrs1", 63}, {"SynBrs2", 64},
  // Reed
  {"Sop Sax", 65}, {"AltoSax", 66}, {"Ten Sax", 67}, {"BariSax", 68},
  {"Oboe", 69}, {"EnglHorn", 70}, {"Bassoon", 71}, {"Clarinet", 72},
  // Pipe
  {"Piccolo", 73}, {"Flute", 74}, {"Recorder", 75}, {"PanFlute", 76},
  {"BlownBottle", 77}, {"Shakuhachi", 78}, {"Whistle", 79}, {"Ocarina", 80},
  // Synth Lead
  {"Ld1Square", 81}, {"Ld2Sawtooth", 82}, {"Ld3Calliope", 83}, {"Ld4Chiff", 84},
  {"Ld5Charang", 85}, {"Ld6Voice", 86}, {"Ld7Fifths", 87}, {"Ld8Bass&Lead", 88},
  // Synth Pad
  {"Pd1NewAge", 89}, {"Pd2Warm", 90}, {"Pd3Polysynth", 91}, {"Pd4Choir", 92},
  {"Pd5BowedGlass", 93}, {"Pd6Metallic", 94}, {"Pd7Halo", 95}, {"Pd8Sweep", 96},
    // Synth Effects
  {"FX1Rain", 97}, {"FX2Soundtrack", 98}, {"FX3Crystal", 99}, {"FX4Atmosphere", 100},
  {"FX5Bright", 101}, {"FX6Goblins", 102}, {"FX7Echoes", 103}, {"FX8SciFi)", 104},
  // Ethnic
  {"Sitar", 105}, {"Banjo", 106}, {"Shamisen", 107}, {"Koto", 108},
  {"Kalimba", 109}, {"BagPipe", 110}, {"Fiddle", 111}, {"Shanai", 112},
  // Percussive
  {"TinkleBell", 113}, {"Cowbell", 114}, {"SteelDrums", 115}, {"WoodBlock", 116},
  {"TaikoDrum", 117}, {"MeloTom", 118}, {"SynDrum", 119}, {"RevCymbal", 120},
  // Sound Effects
  {"GtrFretNoise", 121}, {"BreathNoise", 122}, {"Seashore", 123}, {"BirdTweet", 124},
  {"TelephoneRing", 125}, {"Helicopter", 126}, {"Applause", 127}, {"Gunshot", 128},
};
GEMSelect selectGeneralMidi(sizeof(optionByteGeneralMidi) / sizeof(SelectOptionByte), optionByteGeneralMidi);
GEMItem  menuItemGeneralMidi("GeneralMidi:", programChange,  selectGeneralMidi, sendProgramChange);

GEMSpinnerBoundariesInt optionIntTransposeSteps = { .step = 1, .min = -128, .max = 128};
GEMSpinner selectTransposeSteps(optionIntTransposeSteps);
GEMItem  menuItemTransposeSteps( "Transpose:",   transposeSteps,  selectTransposeSteps, changeTranspose);
  
SelectOptionByte optionByteColor[] =    { { "Rainbow", RAINBOW_MODE }, { "Tiered" , TIERED_COLOR_MODE }, {"Alt", ALTERNATE_COLOR_MODE} };
GEMSelect selectColor( sizeof(optionByteColor) / sizeof(SelectOptionByte), optionByteColor);
GEMItem  menuItemColor( "Color mode:", colorMode, selectColor, setLEDcolorCodes);

SelectOptionByte optionByteAnimate[] =  { { "None" , ANIMATE_NONE }, { "Octave", ANIMATE_OCTAVE },
  { "By Note", ANIMATE_BY_NOTE }, { "Star", ANIMATE_STAR }, { "Splash" , ANIMATE_SPLASH }, { "Orbit", ANIMATE_ORBIT } };
GEMSelect selectAnimate( sizeof(optionByteAnimate)  / sizeof(SelectOptionByte), optionByteAnimate);
GEMItem  menuItemAnimate( "Animation:", animationType, selectAnimate);

SelectOptionByte optionByteBright[] = { { "Off", BRIGHT_OFF}, {"Dimmer", BRIGHT_DIMMER}, {"Dim", BRIGHT_DIM}, {"Low", BRIGHT_LOW}, {"Normal", BRIGHT_MID}, {"High", BRIGHT_HIGH}, {"THE SUN", BRIGHT_MAX } };
GEMSelect selectBright( sizeof(optionByteBright) / sizeof(SelectOptionByte), optionByteBright);
GEMItem menuItemBright( "Brightness", globalBrightness, selectBright, setLEDcolorCodes);

SelectOptionByte optionByteWaveform[] = { { "Hybrid", WAVEFORM_HYBRID }, { "Square", WAVEFORM_SQUARE }, { "Saw", WAVEFORM_SAW },
{"Triangl", WAVEFORM_TRIANGLE}, {"Sine", WAVEFORM_SINE}, {"Strings", WAVEFORM_STRINGS}, {"Clrinet", WAVEFORM_CLARINET} };
GEMSelect selectWaveform(sizeof(optionByteWaveform) / sizeof(SelectOptionByte), optionByteWaveform);
GEMItem  menuItemWaveform( "Waveform:", currWave, selectWaveform, resetSynthFreqs);

SelectOptionInt optionIntModWheel[] = { { "too slo", 1 }, { "Turtle", 2 }, { "Slow", 4 }, 
  { "Medium",    8 }, { "Fast",     16 }, { "Cheetah",  32 }, { "Instant", 127 } };
GEMSelect selectModSpeed(sizeof(optionIntModWheel) / sizeof(SelectOptionInt), optionIntModWheel);
GEMItem  menuItemModSpeed( "Mod wheel:", modWheelSpeed, selectModSpeed);
GEMItem  menuItemVelSpeed( "Vel wheel:", velWheelSpeed, selectModSpeed);

SelectOptionInt optionIntPBWheel[] =  { { "too slo", 128 }, { "Turtle", 256 }, { "Slow", 512 },  
  { "Medium", 1024 }, { "Fast", 2048 }, { "Cheetah", 4096 },  { "Instant", 16384 } };
GEMSelect selectPBSpeed(sizeof(optionIntPBWheel) / sizeof(SelectOptionInt), optionIntPBWheel);
GEMItem  menuItemPBSpeed( "PB wheel:", pbWheelSpeed, selectPBSpeed);

// Call this procedure to return to the main menu
void menuHome() {
  menu.setMenuPageCurrent(menuPageMain);
  menu.drawMenu();
}

void rebootToBootloader() {
  menu.setMenuPageCurrent(menuPageReboot);
  menu.drawMenu();
  strip.clear();
  strip.show();
  rp2040.rebootToBootloader();
}
/*
  This procedure sets each layout menu item to be either
  visible if that layout is available in the current tuning,
  or hidden if not.
  
  It should run once after the layout menu items are
  generated, and then once any time the tuning changes.
*/
void showOnlyValidLayoutChoices() { 
  for (byte L = 0; L < layoutCount; L++) {
    menuItemLayout[L]->hide((layoutOptions[L].tuning != current.tuningIndex));
  }
  sendToLog("menu: Layout choices were updated.");
}
/*
  This procedure sets each scale menu item to be either
  visible if that scale is available in the current tuning,
  or hidden if not.
  
  It should run once after the scale menu items are
  generated, and then once any time the tuning changes.
*/
void showOnlyValidScaleChoices() {
  for (int S = 0; S < scaleCount; S++) {
    menuItemScales[S]->hide((scaleOptions[S].tuning != current.tuningIndex) && (scaleOptions[S].tuning != ALL_TUNINGS));
  }
  sendToLog("menu: Scale choices were updated.");
}
/*
  This procedure sets each key spinner menu item to be either
  visible if the key names correspond to the current tuning,
  or hidden if not.
  
  It should run once after the key selectors are
  generated, and then once any time the tuning changes.
*/
void showOnlyValidKeyChoices() { 
  for (int T = 0; T < TUNINGCOUNT; T++) {
    menuItemKeys[T]->hide((T != current.tuningIndex));
  }
  sendToLog("menu: Key choices were updated.");
}

void updateLayoutAndRotate() {
  applyLayout();
  u8g2.setDisplayRotation(current.layout().isPortrait ? U8G2_R2 : U8G2_R1);     // and landscape / portrait rotation
}
/*
  This procedure is run when a layout is selected via the menu.
  It sets the current layout to the selected value.
  If it's different from the previous one, then
  re-apply the layout to the grid. In any case, go to the
  main menu when done.
*/
void changeLayout(GEMCallbackData callbackData) {
  byte selection = callbackData.valByte;
  if (selection != current.layoutIndex) {
    current.layoutIndex = selection;
    updateLayoutAndRotate();
  }
  menuHome();
}
/*
  This procedure is run when a scale is selected via the menu.
  It sets the current scale to the selected value.
  If it's different from the previous one, then
  re-apply the scale to the grid. In any case, go to the
  main menu when done.
*/
void changeScale(GEMCallbackData callbackData) {   // when you change the scale via the menu
  int selection = callbackData.valInt;
  if (selection != current.scaleIndex) {
    current.scaleIndex = selection;
    applyScale();
  }
  menuHome();
}
/*
  This procedure is run when the key is changed via the menu.
  A key change results in a shift in the location of the
  scale notes relative to the grid.
  In this program, the only thing that occurs is that
  the scale is reapplied to the grid.
  The menu does not go home because the intent is to stay
  on the scale/key screen.
*/
void changeKey() {     // when you change the key via the menu
  applyScale();
}
/*
  This procedure was declared already and is being defined now.
  It's run when the transposition is changed via the menu.
  It sets the current transposition to the selected value.
  The effect of transposition is to change the sounded
  notes but not the layout or display.
  The procedure to re-assign pitches is therefore called.
  The menu doesn't change because the transpose is a spinner select.
*/
void changeTranspose() {     // when you change the transpose via the menu
  current.transpose = transposeSteps;
  assignPitches();
  updateSynthWithNewFreqs();
}
/*
  This procedure is run when the tuning is changed via the menu.
  It affects almost everything in the program, so
  quite a few items are reset, refreshed, and redone
  when the tuning changes.
*/
void changeTuning(GEMCallbackData callbackData) { 
  byte selection = callbackData.valByte;
  if (selection != current.tuningIndex) {
    current.tuningIndex = selection;
    current.layoutIndex = current.layoutsBegin();        // reset layout to first in list
    current.scaleIndex = 0;                              // reset scale to "no scale"
    current.keyStepsFromA = current.tuning().spanCtoA(); // reset key to C
    showOnlyValidLayoutChoices();                        // change list of choices in GEM Menu
    showOnlyValidScaleChoices();                         // change list of choices in GEM Menu
    showOnlyValidKeyChoices();                           // change list of choices in GEM Menu
    updateLayoutAndRotate();   // apply changes above
    resetTuningMIDI();  // clear out MIDI queue
    resetSynthFreqs();
  }
  menuHome();
}
/*
  The procedure below builds menu items for tuning,
  layout, scales, and keys based on what's preloaded.
  We already declared arrays of menu item objects earlier.
  Now we cycle through those arrays, and create GEMItem objects for
  each index. What's nice about doing this in an array is,
  we do not have to assign a variable name to each object; we just
  refer to it by its index in the array.
  
  The constructor "new GEMItem" is populated with the different
  variables in the preset objects we defined earlier.
  Then the menu item is added to the associated page.
  The item must be entered with the asterisk operator
  because an array index technically returns an address in memory
  pointing to the object; the addMenuItem procedure wants
  the contents of that item, which is what the * beforehand does. 
*/
void createTuningMenuItems() {
  for (byte T = 0; T < TUNINGCOUNT; T++) {
    menuItemTuning[T] = new GEMItem(tuningOptions[T].name.c_str(), changeTuning, T);
    menuPageTuning.addMenuItem(*menuItemTuning[T]);
  }
}
void createLayoutMenuItems() {
  for (byte L = 0; L < layoutCount; L++) { // create pointers to all layouts
    menuItemLayout[L] = new GEMItem(layoutOptions[L].name.c_str(), changeLayout, L);
    menuPageLayout.addMenuItem(*menuItemLayout[L]);
  }
  showOnlyValidLayoutChoices();
}
void createKeyMenuItems() {
  for (byte T = 0; T < TUNINGCOUNT; T++) {
    selectKey[T] = new GEMSelect(tuningOptions[T].cycleLength, tuningOptions[T].keyChoices);
    menuItemKeys[T] = new GEMItem("Key:", current.keyStepsFromA, *selectKey[T], changeKey);
    menuPageScales.addMenuItem(*menuItemKeys[T]);
  }
  showOnlyValidKeyChoices();
}
void createScaleMenuItems() {
  for (int S = 0; S < scaleCount; S++) {  // create pointers to all scale items, filter them as you go
    menuItemScales[S] = new GEMItem(scaleOptions[S].name.c_str(), changeScale, S);
    menuPageScales.addMenuItem(*menuItemScales[S]);
  }
  showOnlyValidScaleChoices();
}

void setupMenu() { 
  menu.setSplashDelay(0);
  menu.init();
  /*
    addMenuItem procedure adds that GEM object to the given page.
    The menu items appear in the order they are added,
    so to change the order in the menu change the order in the code.
  */
  menuPageMain.addMenuItem(menuGotoTuning);
    createTuningMenuItems();
    menuPageTuning.addMenuItem(menuTuningBack);
  menuPageMain.addMenuItem(menuGotoLayout);
    createLayoutMenuItems();
    menuPageLayout.addMenuItem(menuLayoutBack);
  menuPageMain.addMenuItem(menuGotoScales);
    createKeyMenuItems();
    menuPageScales.addMenuItem(menuItemScaleLock);
    createScaleMenuItems();
    menuPageScales.addMenuItem(menuScalesBack);
  menuPageMain.addMenuItem(menuGotoControl);
    menuPageControl.addMenuItem(menuItemPBSpeed);
    menuPageControl.addMenuItem(menuItemModSpeed);
    menuPageControl.addMenuItem(menuItemVelSpeed);
    menuPageControl.addMenuItem(menuControlBack);
  menuPageMain.addMenuItem(menuGotoColors);
    menuPageColors.addMenuItem(menuItemColor);
    menuPageColors.addMenuItem(menuItemBright);
    menuPageColors.addMenuItem(menuItemAnimate);
    menuPageColors.addMenuItem(menuColorsBack);
  menuPageMain.addMenuItem(menuGotoSynth);
    menuPageSynth.addMenuItem(menuItemPlayback);  
    menuPageSynth.addMenuItem(menuItemWaveform);
    // menuItemAudioD added here for hardware V1.2
    menuPageSynth.addMenuItem(menuItemRolandMT32);
    menuPageSynth.addMenuItem(menuItemGeneralMidi);
    menuPageSynth.addMenuItem(menuSynthBack);
  menuPageMain.addMenuItem(menuItemTransposeSteps);
  menuPageMain.addMenuItem(menuGotoAdvanced);
    menuPageAdvanced.addMenuItem(menuItemVersion);
    menuPageAdvanced.addMenuItem(menuItemHardware);
    menuPageAdvanced.addMenuItem(menuItemMPEpitchBend);
    menuPageAdvanced.addMenuItem(menuItemRotary);
    menuPageAdvanced.addMenuItem(menuItemPercep);
    menuPageAdvanced.addMenuItem(menuItemShiftColor);
    menuPageAdvanced.addMenuItem(menuItemWheelAlt);
    menuPageAdvanced.addMenuItem(menuItemPBBehave);
    menuPageAdvanced.addMenuItem(menuItemModBehave);
    menuPageAdvanced.addMenuItem(menuItemUSBBootloader);
    menuPageAdvanced.addMenuItem(menuAdvancedBack);
  menuHome();
}

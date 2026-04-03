class EmConstants {
  EmConstants._();

  // ─── GM Program Names (0-127) ────────────────────────────────────────────
  static const List<String> gmProgramNames = [
    'Acoustic Grand Piano',  // 0
    'Bright Acoustic Piano', // 1
    'Electric Grand Piano',  // 2
    'Honky-tonk Piano',      // 3
    'Electric Piano 1',      // 4
    'Electric Piano 2',      // 5
    'Harpsichord',           // 6
    'Clavinet',              // 7
    'Celesta',               // 8
    'Glockenspiel',          // 9
    'Music Box',             // 10
    'Vibraphone',            // 11
    'Marimba',               // 12
    'Xylophone',             // 13
    'Tubular Bells',         // 14
    'Dulcimer',              // 15
    'Drawbar Organ',         // 16
    'Percussive Organ',      // 17
    'Rock Organ',            // 18
    'Church Organ',          // 19
    'Reed Organ',            // 20
    'Accordion',             // 21
    'Harmonica',             // 22
    'Tango Accordion',       // 23
    'Nylon String Guitar',   // 24
    'Steel String Guitar',   // 25
    'Jazz Guitar',           // 26
    'Clean Guitar',          // 27
    'Muted Guitar',          // 28
    'Overdriven Guitar',     // 29
    'Distortion Guitar',     // 30
    'Guitar Harmonics',      // 31
    'Acoustic Bass',         // 32
    'Finger Bass',           // 33
    'Pick Bass',             // 34
    'Fretless Bass',         // 35
    'Slap Bass 1',           // 36
    'Slap Bass 2',           // 37
    'Synth Bass 1',          // 38
    'Synth Bass 2',          // 39
    'Violin',                // 40
    'Viola',                 // 41
    'Cello',                 // 42
    'Contrabass',            // 43
    'Tremolo Strings',       // 44
    'Pizzicato Strings',     // 45
    'Orchestral Harp',       // 46
    'Timpani',               // 47
    'String Ensemble 1',     // 48
    'String Ensemble 2',     // 49
    'Synth Strings 1',       // 50
    'Synth Strings 2',       // 51
    'Choir Aahs',            // 52
    'Voice Oohs',            // 53
    'Synth Voice',           // 54
    'Orchestra Hit',         // 55
    'Trumpet',               // 56
    'Trombone',              // 57
    'Tuba',                  // 58
    'Muted Trumpet',         // 59
    'French Horn',           // 60
    'Brass Section',         // 61
    'Synth Brass 1',         // 62
    'Synth Brass 2',         // 63
    'Soprano Sax',           // 64
    'Alto Sax',              // 65
    'Tenor Sax',             // 66
    'Baritone Sax',          // 67
    'Oboe',                  // 68
    'English Horn',          // 69
    'Bassoon',               // 70
    'Clarinet',              // 71
    'Piccolo',               // 72
    'Flute',                 // 73
    'Recorder',              // 74
    'Pan Flute',             // 75
    'Blown Bottle',          // 76
    'Shakuhachi',            // 77
    'Whistle',               // 78
    'Ocarina',               // 79
    'Square Lead',           // 80
    'Sawtooth Lead',         // 81
    'Calliope Lead',         // 82
    'Chiff Lead',            // 83
    'Charang Lead',          // 84
    'Voice Lead',            // 85
    'Fifths Lead',           // 86
    'Bass+Lead',             // 87
    'New Age Pad',           // 88
    'Warm Pad',              // 89
    'Polysynth Pad',         // 90
    'Choir Pad',             // 91
    'Bowed Pad',             // 92
    'Metallic Pad',          // 93
    'Halo Pad',              // 94
    'Sweep Pad',             // 95
    'Rain FX',               // 96
    'Soundtrack FX',         // 97
    'Crystal FX',            // 98
    'Atmosphere FX',         // 99
    'Brightness FX',         // 100
    'Goblins FX',            // 101
    'Echoes FX',             // 102
    'Sci-fi FX',             // 103
    'Sitar',                 // 104
    'Banjo',                 // 105
    'Shamisen',              // 106
    'Koto',                  // 107
    'Kalimba',               // 108
    'Bag Pipe',              // 109
    'Fiddle',                // 110
    'Shanai',                // 111
    'Tinkle Bell',           // 112
    'Agogo',                 // 113
    'Steel Drums',           // 114
    'Woodblock',             // 115
    'Taiko Drum',            // 116
    'Melodic Tom',           // 117
    'Synth Drum',            // 118
    'Reverse Cymbal',        // 119
    'Guitar Fret Noise',     // 120
    'Breath Noise',          // 121
    'Seashore',              // 122
    'Bird Tweet',            // 123
    'Telephone Ring',        // 124
    'Helicopter',            // 125
    'Applause',              // 126
    'Gunshot',               // 127
  ];

  // ─── Drum Instrument Names (12 rows) ────────────────────────────────────
  static const List<String> drumInstrumentNames = [
    'BD',   // Bass Drum
    'SD',   // Snare Drum
    'SS',   // Side Stick
    'cHH',  // Closed Hi-Hat
    'oHH',  // Open Hi-Hat
    'pHH',  // Pedal Hi-Hat
    'CR',   // Crash Cymbal
    'RD',   // Ride Cymbal
    'LT',   // Low Tom
    'HT',   // High Tom
    'CLP',  // Clap
    'CB',   // Cowbell
  ];

  static const List<String> drumInstrumentFullNames = [
    'Bass Drum',
    'Snare Drum',
    'Side Stick',
    'Closed Hi-Hat',
    'Open Hi-Hat',
    'Pedal Hi-Hat',
    'Crash Cymbal',
    'Ride Cymbal',
    'Low Tom',
    'High Tom',
    'Clap',
    'Cowbell',
  ];

  // ─── Voice Slot Role Names (8 slots) ─────────────────────────────────────
  static const List<String> slotRoleNames = [
    'UPPER 1',
    'UPPER 2',
    'LOWER 1',
    'LOWER 2',
    'LEAD 1',
    'LEAD 2',
    'PEDAL 1',
    'PEDAL 2',
  ];

  static const List<String> slotRoleNamesCn = [
    '上键盘1',
    '上键盘2',
    '下键盘1',
    '下键盘2',
    '旋律1',
    '旋律2',
    '脚踏1',
    '脚踏2',
  ];

  // ─── Voice Category Labels ────────────────────────────────────────────────
  static const List<Map<String, String>> voiceCategories = [
    {'cn': '弦乐', 'en': 'Strings'},
    {'cn': '铜管', 'en': 'Brass'},
    {'cn': '木管', 'en': 'Woodwind'},
    {'cn': '合奏', 'en': 'Ensemble'},
    {'cn': '延音', 'en': 'Sustain'},
    {'cn': '合成', 'en': 'Synth'},
    {'cn': '钢琴', 'en': 'Piano'},
    {'cn': '管风琴', 'en': 'Organ'},
    {'cn': '打击乐', 'en': 'Perc'},
    {'cn': '吉他', 'en': 'Guitar'},
    {'cn': '合唱', 'en': 'Choir'},
    {'cn': '民族', 'en': 'Ethnic'},
  ];

  // ─── Rhythm Preset Names ──────────────────────────────────────────────────
  static const List<String> rhythmPresetNames = [
    'Rock 1',
    'Rock 2',
    'Pop',
    'Jazz Swing',
    'Bossa Nova',
    'Waltz',
    'March',
    'Ballad',
    'Funk',
    'Latin',
    'Samba',
    'Reggae',
    'Blues',
    'Country',
    'Electronic',
    'Hip-Hop',
  ];

  // ─── LFO Wave Names ───────────────────────────────────────────────────────
  static const List<String> lfoWaveNames = [
    'Sine',
    'Sawtooth',
    'Triangle',
    'Square',
    'Random',
  ];

  // ─── Layout Breakpoints ───────────────────────────────────────────────────
  static const double breakpointPhone = 600.0;
  static const double breakpointTablet = 900.0;
  static const double breakpointDesktop = 1100.0;

  static const int drumRows = 12;
  static const int drumSteps = 16;
  static const int voiceSlotCount = 8;

  // ─── XG Voice Database (Yamaha Extended) ────────────────────────────────────
  // Format: {bankMSB, bankLSB, program, 'cn_name', 'en_name', 'category'}
  static const List<Map<dynamic, dynamic>> xgVoices = [
    // Bank 0 (GM Piano) - Programs 0-7
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 0,
      'cn': '三角钢琴',
      'en': 'Acoustic Grand Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 1,
      'cn': '明亮钢琴',
      'en': 'Bright Acoustic Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 2,
      'cn': '电钢琴',
      'en': 'Electric Grand Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 3,
      'cn': '酒吧钢琴',
      'en': 'Honky-tonk Piano',
      'cat': 'Piano'
    },
    // Bank 0 - Electric Pianos (Programs 4-7)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 4,
      'cn': '电钢琴1',
      'en': 'Electric Piano 1',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 5,
      'cn': '电钢琴2',
      'en': 'Electric Piano 2',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 6,
      'cn': '羽管键琴',
      'en': 'Harpsichord',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 7,
      'cn': '古钢琴',
      'en': 'Clavinet',
      'cat': 'Piano'
    },
    // Bank 0 - Chromatic Percussion (Programs 8-15)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 8,
      'cn': '钟琴',
      'en': 'Celesta',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 9,
      'cn': '钟琴2',
      'en': 'Glockenspiel',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 10,
      'cn': '音乐盒',
      'en': 'Music Box',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 11,
      'cn': '颤音琴',
      'en': 'Vibraphone',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 12,
      'cn': '马林巴琴',
      'en': 'Marimba',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 13,
      'cn': '木琴',
      'en': 'Xylophone',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 14,
      'cn': '管状钟',
      'en': 'Tubular Bells',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 15,
      'cn': '公爵琴',
      'en': 'Dulcimer',
      'cat': 'Perc'
    },
    // Bank 0 - Organ (Programs 16-23)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 16,
      'cn': '抽屉式风琴',
      'en': 'Drawbar Organ',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 17,
      'cn': '打击式风琴',
      'en': 'Percussive Organ',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 18,
      'cn': '摇滚风琴',
      'en': 'Rock Organ',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 19,
      'cn': '教堂风琴',
      'en': 'Church Organ',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 20,
      'cn': '簧管风琴',
      'en': 'Reed Organ',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 21,
      'cn': '手风琴',
      'en': 'Accordion',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 22,
      'cn': '口琴',
      'en': 'Harmonica',
      'cat': 'Organ'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 23,
      'cn': '探戈手风琴',
      'en': 'Tango Accordion',
      'cat': 'Organ'
    },
    // Bank 0 - Guitar (Programs 24-31)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 24,
      'cn': '尼龙吉他',
      'en': 'Nylon String Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 25,
      'cn': '钢弦吉他',
      'en': 'Steel String Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 26,
      'cn': '爵士吉他',
      'en': 'Jazz Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 27,
      'cn': '清晰吉他',
      'en': 'Clean Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 28,
      'cn': '闷音吉他',
      'en': 'Muted Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 29,
      'cn': '过度驱动吉他',
      'en': 'Overdriven Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 30,
      'cn': '失真吉他',
      'en': 'Distortion Guitar',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 31,
      'cn': '吉他谐音',
      'en': 'Guitar Harmonics',
      'cat': 'Guitar'
    },
    // Bank 0 - Bass (Programs 32-39)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 32,
      'cn': '原声贝司',
      'en': 'Acoustic Bass',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 33,
      'cn': '指拨贝司',
      'en': 'Finger Bass',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 34,
      'cn': '拨片贝司',
      'en': 'Pick Bass',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 35,
      'cn': '无品贝司',
      'en': 'Fretless Bass',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 36,
      'cn': '拍奏贝司1',
      'en': 'Slap Bass 1',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 37,
      'cn': '拍奏贝司2',
      'en': 'Slap Bass 2',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 38,
      'cn': '合成贝司1',
      'en': 'Synth Bass 1',
      'cat': 'Guitar'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 39,
      'cn': '合成贝司2',
      'en': 'Synth Bass 2',
      'cat': 'Guitar'
    },
    // Bank 0 - Strings (Programs 40-47)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 40,
      'cn': '小提琴',
      'en': 'Violin',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 41,
      'cn': '中提琴',
      'en': 'Viola',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 42,
      'cn': '大提琴',
      'en': 'Cello',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 43,
      'cn': '低音提琴',
      'en': 'Contrabass',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 44,
      'cn': '弦乐颤音',
      'en': 'Tremolo Strings',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 45,
      'cn': '弦乐拨奏',
      'en': 'Pizzicato Strings',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 46,
      'cn': '竖琴',
      'en': 'Orchestral Harp',
      'cat': 'Strings'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 47,
      'cn': '定音鼓',
      'en': 'Timpani',
      'cat': 'Strings'
    },
    // Bank 0 - Ensemble (Programs 48-55)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 48,
      'cn': '弦乐组1',
      'en': 'String Ensemble 1',
      'cat': 'Ensemble'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 49,
      'cn': '弦乐组2',
      'en': 'String Ensemble 2',
      'cat': 'Ensemble'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 50,
      'cn': '合成弦1',
      'en': 'Synth Strings 1',
      'cat': 'Ensemble'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 51,
      'cn': '合成弦2',
      'en': 'Synth Strings 2',
      'cat': 'Ensemble'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 52,
      'cn': '合唱啊声',
      'en': 'Choir Aahs',
      'cat': 'Choir'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 53,
      'cn': '人声哦声',
      'en': 'Voice Oohs',
      'cat': 'Choir'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 54,
      'cn': '合成人声',
      'en': 'Synth Voice',
      'cat': 'Choir'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 55,
      'cn': '管弦乐敲击',
      'en': 'Orchestra Hit',
      'cat': 'Ensemble'
    },
    // Bank 0 - Brass (Programs 56-63)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 56,
      'cn': '小号',
      'en': 'Trumpet',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 57,
      'cn': '长号',
      'en': 'Trombone',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 58,
      'cn': '大号',
      'en': 'Tuba',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 59,
      'cn': '弱音小号',
      'en': 'Muted Trumpet',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 60,
      'cn': '法国号',
      'en': 'French Horn',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 61,
      'cn': '铜管乐组',
      'en': 'Brass Section',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 62,
      'cn': '合成铜管1',
      'en': 'Synth Brass 1',
      'cat': 'Brass'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 63,
      'cn': '合成铜管2',
      'en': 'Synth Brass 2',
      'cat': 'Brass'
    },
    // Bank 0 - Reed (Programs 64-71)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 64,
      'cn': '高音萨克斯',
      'en': 'Soprano Sax',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 65,
      'cn': '中音萨克斯',
      'en': 'Alto Sax',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 66,
      'cn': '次中音萨克斯',
      'en': 'Tenor Sax',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 67,
      'cn': '低音萨克斯',
      'en': 'Baritone Sax',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 68,
      'cn': '双簧管',
      'en': 'Oboe',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 69,
      'cn': '英国管',
      'en': 'English Horn',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 70,
      'cn': '大管',
      'en': 'Bassoon',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 71,
      'cn': '单簧管',
      'en': 'Clarinet',
      'cat': 'Woodwind'
    },
    // Bank 0 - Pipe (Programs 72-79)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 72,
      'cn': '短笛',
      'en': 'Piccolo',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 73,
      'cn': '长笛',
      'en': 'Flute',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 74,
      'cn': '竖笛',
      'en': 'Recorder',
      'cat': 'Woodwind'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 75,
      'cn': '排箫',
      'en': 'Pan Flute',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 76,
      'cn': '吹瓶',
      'en': 'Blown Bottle',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 77,
      'cn': '尺八',
      'en': 'Shakuhachi',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 78,
      'cn': '口笛',
      'en': 'Whistle',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 79,
      'cn': '埙',
      'en': 'Ocarina',
      'cat': 'Ethnic'
    },
    // Bank 0 - Synth Lead (Programs 80-87)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 80,
      'cn': '方波铅音',
      'en': 'Square Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 81,
      'cn': '锯齿波铅音',
      'en': 'Sawtooth Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 82,
      'cn': '卡利欧普铅音',
      'en': 'Calliope Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 83,
      'cn': '切割音铅音',
      'en': 'Chiff Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 84,
      'cn': '恰朗铅音',
      'en': 'Charang Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 85,
      'cn': '人声铅音',
      'en': 'Voice Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 86,
      'cn': '五度铅音',
      'en': 'Fifths Lead',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 87,
      'cn': '贝司+铅音',
      'en': 'Bass+Lead',
      'cat': 'Synth'
    },
    // Bank 0 - Synth Pad (Programs 88-95)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 88,
      'cn': '新世纪垫音',
      'en': 'New Age Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 89,
      'cn': '温暖垫音',
      'en': 'Warm Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 90,
      'cn': '聚合成垫音',
      'en': 'Polysynth Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 91,
      'cn': '合唱垫音',
      'en': 'Choir Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 92,
      'cn': '弓弦垫音',
      'en': 'Bowed Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 93,
      'cn': '金属垫音',
      'en': 'Metallic Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 94,
      'cn': '光环垫音',
      'en': 'Halo Pad',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 95,
      'cn': '扫过垫音',
      'en': 'Sweep Pad',
      'cat': 'Synth'
    },
    // Bank 0 - Synth Effects (Programs 96-103)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 96,
      'cn': '雨声效果',
      'en': 'Rain FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 97,
      'cn': '电影效果',
      'en': 'Soundtrack FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 98,
      'cn': '水晶效果',
      'en': 'Crystal FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 99,
      'cn': '大气效果',
      'en': 'Atmosphere FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 100,
      'cn': '亮度效果',
      'en': 'Brightness FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 101,
      'cn': '妖怪效果',
      'en': 'Goblins FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 102,
      'cn': '回声效果',
      'en': 'Echoes FX',
      'cat': 'Synth'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 103,
      'cn': '科幻效果',
      'en': 'Sci-fi FX',
      'cat': 'Synth'
    },
    // Bank 0 - Ethnic (Programs 104-111)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 104,
      'cn': '西塔琴',
      'en': 'Sitar',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 105,
      'cn': '班卓琴',
      'en': 'Banjo',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 106,
      'cn': '三味线',
      'en': 'Shamisen',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 107,
      'cn': '古筝',
      'en': 'Koto',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 108,
      'cn': '卡林巴琴',
      'en': 'Kalimba',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 109,
      'cn': '风笛',
      'en': 'Bag Pipe',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 110,
      'cn': '小提琴',
      'en': 'Fiddle',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 111,
      'cn': '山奈琴',
      'en': 'Shanai',
      'cat': 'Ethnic'
    },
    // Bank 0 - Percussive (Programs 112-119)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 112,
      'cn': '铃铛',
      'en': 'Tinkle Bell',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 113,
      'cn': '阿哥哥',
      'en': 'Agogo',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 114,
      'cn': '钢鼓',
      'en': 'Steel Drums',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 115,
      'cn': '木块',
      'en': 'Woodblock',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 116,
      'cn': '太鼓',
      'en': 'Taiko Drum',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 117,
      'cn': '旋律汤姆',
      'en': 'Melodic Tom',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 118,
      'cn': '合成鼓',
      'en': 'Synth Drum',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 119,
      'cn': '反向钹',
      'en': 'Reverse Cymbal',
      'cat': 'Perc'
    },
    // Bank 0 - Sound Effects (Programs 120-127)
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 120,
      'cn': '吉他噪音',
      'en': 'Guitar Fret Noise',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 121,
      'cn': '呼吸声',
      'en': 'Breath Noise',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 122,
      'cn': '海浪声',
      'en': 'Seashore',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 123,
      'cn': '鸟鸣声',
      'en': 'Bird Tweet',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 124,
      'cn': '电话铃声',
      'en': 'Telephone Ring',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 125,
      'cn': '直升机声',
      'en': 'Helicopter',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 126,
      'cn': '掌声',
      'en': 'Applause',
      'cat': 'Perc'
    },
    {
      'bankMSB': 0,
      'bankLSB': 0,
      'program': 127,
      'cn': '枪声',
      'en': 'Gunshot',
      'cat': 'Perc'
    },
    // Bank 1-7 (XG Extended Banks) - Sample common Yamaha extended voices
    // Bank 1: Piano variations
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 0,
      'cn': '欧洲钢琴',
      'en': 'European Grand Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 1,
      'cn': '哈默施坦钢琴',
      'en': 'Hammerstein Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 2,
      'cn': '钢铁钢琴',
      'en': 'Steel Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 3,
      'cn': '喜剧钢琴',
      'en': 'Comedy Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 4,
      'cn': '破旧钢琴',
      'en': 'Detuned CP78',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 5,
      'cn': '暗黑钢琴',
      'en': 'Dark Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 6,
      'cn': '玻璃钢琴',
      'en': 'Glass Piano',
      'cat': 'Piano'
    },
    {
      'bankMSB': 0,
      'bankLSB': 1,
      'program': 7,
      'cn': '沙漠钢琴',
      'en': 'Ambient Piano',
      'cat': 'Piano'
    },
    // Bank 2: Ethnic Piano variations
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 0,
      'cn': '中国古琴',
      'en': 'Guqin',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 1,
      'cn': '中国琵琶',
      'en': 'Pipa',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 2,
      'cn': '中国笛',
      'en': 'Dizi',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 3,
      'cn': '中国笙',
      'en': 'Sheng',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 4,
      'cn': '中国京胡',
      'en': 'Jinghu',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 5,
      'cn': '日本琴',
      'en': 'Koto',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 6,
      'cn': '日本尺八',
      'en': 'Shakuhachi',
      'cat': 'Ethnic'
    },
    {
      'bankMSB': 0,
      'bankLSB': 2,
      'program': 7,
      'cn': '日本太鼓',
      'en': 'Taiko Drum',
      'cat': 'Ethnic'
    },
  ];
}

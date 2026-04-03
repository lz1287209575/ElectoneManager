import 'package:flutter/material.dart';

class EmColors {
  EmColors._();

  static const Color background = Color(0xFF0F1118);
  static const Color surface = Color(0xFF202228);
  static const Color surfaceVariant = Color(0xFF2A2D35);
  static const Color cardUpper = Color(0xFF1A3820);
  static const Color cardLower = Color(0xFF38201A);
  static const Color cardLead = Color(0xFF383020);
  static const Color cardPedal = Color(0xFF302820);
  static const Color accent = Color(0xFFFFAA33);
  static const Color accentCyan = Color(0xFF33CCFF);
  static const Color voiceButton = Color(0xFFB44128);
  static const Color textPrimary = Color(0xFFE0E0E0);
  static const Color textSecondary = Color(0xFF808080);
  static const Color textDim = Color(0xFF505060);
  static const Color ledGreen = Color(0xFF00FF00);
  static const Color ledRed = Color(0xFFFF3333);
  static const Color ledCyan = Color(0xFF33CCFF);
  static const Color ledOff = Color(0xFF303030);
  static const Color divider = Color(0xFF303540);
  static const Color sliderTrack = Color(0xFF404550);

  /// Drum grid row colors: BD, SD, SS, cHH, oHH, pHH, CR, RD, LT, HT, CLP, CB
  static const List<Color> drumRowColors = [
    Color(0xFFFF4444), // BD  - red
    Color(0xFFFF8C44), // SD  - orange
    Color(0xFFFFCC44), // SS  - yellow
    Color(0xFF88FF44), // cHH - lime
    Color(0xFF44FF88), // oHH - green
    Color(0xFF44FFCC), // pHH - teal
    Color(0xFF44CCFF), // CR  - sky blue
    Color(0xFF4488FF), // RD  - blue
    Color(0xFF8844FF), // LT  - purple
    Color(0xFFCC44FF), // HT  - violet
    Color(0xFFFF44CC), // CLP - pink
    Color(0xFFFF4488), // CB  - rose
  ];
}

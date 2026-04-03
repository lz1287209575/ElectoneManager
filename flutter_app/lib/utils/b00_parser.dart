import 'dart:io';
import 'dart:async';

/// Yamaha ELS B00 registration file parser.
///
/// A B00 file is a sequence of MIDI SysEx messages (F0...F7) that, when sent
/// to an ELS-02C, restore a full registration (voices, effects, parameters).
///
/// Typical structure:
///   - 1 large bulk dump (F0 43 70 78 ...) — internal ELS memory snapshot
///   - 48 registration groups, each containing:
///     - XG System On reset (F0 05 7E 7F 09 01 F7)
///     - XG Parameter Change messages (F0 08 43 10 4C ...)
///     - Yamaha ELS-specific messages (F0 xx 43 73/76 ...)
///
/// Usage:
///   final result = await B00Parser.parse('/path/to/REG_001.B00');
///   // Send each SysEx message to the piano via MIDI output
class B00ParseResult {
  const B00ParseResult({
    required this.messages,
    required this.registrationCount,
    required this.totalBytes,
    this.error,
  });

  /// All SysEx messages extracted from the file (including the bulk dump).
  final List<List<int>> messages;

  /// Number of Registration sections found (each separated by XG System On).
  final int registrationCount;

  /// Total file size in bytes.
  final int totalBytes;

  /// Non-null if parsing failed.
  final String? error;

  bool get isValid => error == null && messages.isNotEmpty;

  /// Messages for a specific registration index (0-based).
  /// Returns the SysEx messages between two consecutive XG System On events.
  List<List<int>> registrationMessages(int regIndex) {
    final sysOnIndices = <int>[];
    for (int i = 0; i < messages.length; i++) {
      final m = messages[i];
      // XG System On: F0 05 7E 7F 09 01 F7
      if (m.length == 7 &&
          m[0] == 0xF0 &&
          m[1] == 0x05 &&
          m[2] == 0x7E &&
          m[3] == 0x7F) {
        sysOnIndices.add(i);
      }
    }
    if (regIndex < 0 || regIndex >= sysOnIndices.length) return [];
    final start = sysOnIndices[regIndex];
    final end = regIndex + 1 < sysOnIndices.length
        ? sysOnIndices[regIndex + 1]
        : messages.length;
    return messages.sublist(start, end);
  }
}

class B00Parser {
  /// Parse a B00 file and extract all SysEx messages.
  static Future<B00ParseResult> parse(String filePath) async {
    try {
      final file = File(filePath);
      if (!await file.exists()) {
        return B00ParseResult(
          messages: [],
          registrationCount: 0,
          totalBytes: 0,
          error: 'File not found: $filePath',
        );
      }

      final data = await file.readAsBytes();
      return _parseBytes(data);
    } catch (e) {
      return B00ParseResult(
        messages: [],
        registrationCount: 0,
        totalBytes: 0,
        error: 'Parse error: $e',
      );
    }
  }

  static B00ParseResult _parseBytes(List<int> data) {
    final messages = <List<int>>[];

    // Extract all F0...F7 SysEx messages
    int i = 0;
    while (i < data.length) {
      if (data[i] == 0xF0) {
        int j = i + 1;
        while (j < data.length && data[j] != 0xF7) {
          j++;
        }
        if (j < data.length) {
          // Include both F0 and F7
          messages.add(List<int>.unmodifiable(data.sublist(i, j + 1)));
        }
        i = j + 1;
      } else {
        i++;
      }
    }

    // Count registration groups (bounded by XG System On messages)
    int regCount = 0;
    for (final m in messages) {
      if (m.length == 7 &&
          m[0] == 0xF0 &&
          m[1] == 0x05 &&
          m[2] == 0x7E &&
          m[3] == 0x7F) {
        regCount++;
      }
    }

    if (messages.isEmpty) {
      return B00ParseResult(
        messages: [],
        registrationCount: 0,
        totalBytes: data.length,
        error: 'No SysEx messages found in file',
      );
    }

    return B00ParseResult(
      messages: messages,
      registrationCount: regCount,
      totalBytes: data.length,
    );
  }

  /// Check if a file looks like a valid B00 file (quick header check).
  static Future<bool> isValidB00(String filePath) async {
    try {
      final file = File(filePath);
      if (!await file.exists()) return false;
      final bytes = await file.openRead(0, 8).first;
      // B00 starts with F0 43 70 78 (Yamaha bulk dump) or another SysEx
      return bytes.isNotEmpty && bytes[0] == 0xF0;
    } catch (_) {
      return false;
    }
  }
}

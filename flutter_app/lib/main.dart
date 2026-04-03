import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:electone_manager/screens/home_screen.dart';
import 'package:electone_manager/utils/colors.dart';

void main() {
  runApp(const ProviderScope(child: MyApp()));
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Electone Manager',
      theme: ThemeData(
        useMaterial3: true,
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.amber,
          brightness: Brightness.dark,
        ),
        scaffoldBackgroundColor: EmColors.background,
        appBarTheme: AppBarTheme(
          backgroundColor: EmColors.surface,
          foregroundColor: EmColors.textPrimary,
        ),
      ),
      home: const HomeScreen(),
    );
  }
}

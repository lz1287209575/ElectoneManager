# ElectoneManager

跨平台原生 C++ 音乐框架，连接 Yamaha ELS-02C，实现音色触发与 USB 音频回传。目标延迟 < 10ms。

## 架构概览

```
src/
├── common/          # 跨平台抽象层（接口 + 核心算法）
│   ├── IMidiDevice.h
│   ├── IAudioEngine.h
│   ├── MidiMessage.h
│   ├── RingBuffer.h
│   ├── AudioDeviceScanner.h
│   ├── MidiToChineseInstrumentMapper.h/.cpp
│   └── SamplerEngine.h/.cpp
└── platform/
    ├── apple/       # Core MIDI + Core Audio (macOS / iOS)
    ├── windows/     # WinMM MIDI + ASIO / WASAPI
    ├── linux/       # ALSA Seq MIDI + ALSA PCM
    └── android/     # AMidi NDK + Oboe
```

### 线程模型
| 线程 | 职责 |
|------|------|
| MIDI 线程（平台驱动）| 接收 MIDI 数据，推入 lock-free RingBuffer |
| Processing 线程 | 消费 RingBuffer → MidiToChineseInstrumentMapper → 更新 SamplerEngine 参数 |
| Audio RT 线程（平台驱动）| `SamplerEngine::render()` → 输出 PCM |

---

## 构建：macOS

```bash
brew install cmake
cd ElectoneManager
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
./build/ElectoneManager
```

---

## 构建：Windows

要求：
- CMake ≥ 3.21
- Visual Studio 2022（MSVC C++17）
- **ASIO SDK**（可选，极限低延迟）：从 https://www.steinberg.net/developers/ 下载，解压到 `third_party/asiosdk/`

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\ElectoneManager.exe
```

---

## 构建：Linux

```bash
sudo apt install cmake libasound2-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/ElectoneManager
```

**注意**：建议直接使用 ALSA，避开 PulseAudio/PipeWire 混音延迟。

---

## 构建：Android

要求：
- Android NDK r25+（API Level 29+）
- CMake ≥ 3.21
- Oboe 库：`git submodule add https://github.com/google/oboe third_party/oboe`

```bash
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DEM_ANDROID=1
cmake --build build-android -j$(nproc)
```

权限配置已在 `config/AndroidManifest.xml` 中预置。

---

## 关键参数

| 参数 | 默认值 | CMake 变量 |
|------|--------|------------|
| 缓冲大小 | 256 frames（~5.3ms） | `-DEM_BUFFER_SIZE=256` |
| 采样率 | 48000 Hz | `-DEM_SAMPLE_RATE=48000` |

---

## 核心算法

### 14-bit Pitch Bend（水平触前）解析
```
raw = (MSB << 7) | LSB   // 0-16383，中心 8192
float = (raw - 8192) / 8192.0   // -1.0 to +1.0
```

### 低通滤波器（SamplerEngine）
一阶 EMA：`output = α × input + (1-α) × prev`，截止频率由 CC#74（Brightness）控制。

---

## 平台注意事项

| 平台 | 注意 |
|------|------|
| Windows | ASIO 需要下载 Steinberg ASIO SDK 并放到 `third_party/asiosdk/` |
| iOS | `config/Info.plist` 已包含麦克风 + 蓝牙 MIDI 权限 |
| Android | `config/AndroidManifest.xml` 已包含 RECORD_AUDIO + MIDI 权限 |
| Linux | 建议 `RLIMIT_MEMLOCK` 解锁，或以 `CAP_SYS_NICE` 运行以允许 SCHED_FIFO |

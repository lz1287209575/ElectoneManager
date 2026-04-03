// ScorePage.swift — Tab 2: MIDI file loader + progress + controls
import SwiftUI
import UniformTypeIdentifiers

struct ScorePage: View {
    @Environment(EngineController.self) private var engine
    @State private var showFilePicker = false

    var body: some View {
        VStack(spacing: 20) {
            // Header
            Text("MIDI 乐谱")
                .font(.title2.bold())
                .foregroundStyle(.white)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(.horizontal)

            // File picker button
            Button {
                showFilePicker = true
            } label: {
                HStack {
                    Image(systemName: "folder.badge.plus")
                    Text(engine.midiLoaded ? "更换文件" : "载入 MIDI 文件")
                }
                .font(.headline)
                .frame(maxWidth: .infinity)
                .padding()
                .background(Color.blue.opacity(0.3))
                .foregroundStyle(.white)
                .cornerRadius(12)
                .overlay(RoundedRectangle(cornerRadius: 12).stroke(Color.blue, lineWidth: 1))
            }
            .padding(.horizontal)
            .fileImporter(isPresented: $showFilePicker,
                          allowedContentTypes: [UTType(filenameExtension: "mid") ?? .data,
                                                UTType(filenameExtension: "midi") ?? .data],
                          onCompletion: handleFilePick)

            // Progress section
            if engine.midiLoaded {
                VStack(spacing: 8) {
                    // Seek slider
                    Slider(value: Binding(
                        get: { Double(engine.midiProgress) },
                        set: { engine.seekMidi(fraction: Float($0)) }
                    ), in: 0...1)
                    .tint(.cyan)
                    .padding(.horizontal)

                    // Time display
                    HStack {
                        Text(formatProgress(engine.midiProgress))
                            .foregroundStyle(.gray)
                            .font(.caption.monospaced())
                        Spacer()
                        Text("100%")
                            .foregroundStyle(.gray)
                            .font(.caption.monospaced())
                    }
                    .padding(.horizontal)
                }

                // Playback controls
                HStack(spacing: 24) {
                    // Rewind
                    Button {
                        engine.rewindMidi()
                    } label: {
                        Image(systemName: "backward.end.fill")
                            .font(.title2)
                            .foregroundStyle(.white)
                    }

                    // Play / Pause
                    Button {
                        engine.play()
                    } label: {
                        Image(systemName: engine.isPlaying ? "pause.circle.fill" : "play.circle.fill")
                            .font(.system(size: 56))
                            .foregroundStyle(.cyan)
                    }

                    // Stop
                    Button {
                        engine.stop_transport()
                    } label: {
                        Image(systemName: "stop.circle.fill")
                            .font(.title2)
                            .foregroundStyle(.red)
                    }
                }
                .padding()
                .frame(maxWidth: .infinity)
                .background(Color.white.opacity(0.05))
                .cornerRadius(16)
                .padding(.horizontal)

            } else {
                // Empty state
                VStack(spacing: 12) {
                    Image(systemName: "music.note.list")
                        .font(.system(size: 64))
                        .foregroundStyle(.gray)
                    Text("尚未载入乐谱")
                        .foregroundStyle(.gray)
                    Text("支持标准 MIDI Format 0/1 文件")
                        .font(.caption)
                        .foregroundStyle(.gray.opacity(0.6))
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .padding(.top, 60)
            }

            Spacer()
        }
        .padding(.top)
    }

    private func handleFilePick(result: Result<URL, Error>) {
        switch result {
        case .success(let url):
            guard url.startAccessingSecurityScopedResource() else { return }
            defer { url.stopAccessingSecurityScopedResource() }
            engine.loadMidi(url: url)
        case .failure(let err):
            print("File pick error: \(err)")
        }
    }

    private func formatProgress(_ p: Float) -> String {
        let pct = Int(p * 100)
        return "\(pct)%"
    }
}

#Preview {
    ScorePage().environment(EngineController.shared)
}

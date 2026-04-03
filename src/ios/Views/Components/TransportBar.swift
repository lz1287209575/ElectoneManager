// TransportBar.swift — Persistent bottom bar: Play/Stop + Tempo + Bar:Beat
import SwiftUI

struct TransportBar: View {
    @Environment(EngineController.self) private var engine
    @State private var tempoInput: Double = 120

    var body: some View {
        VStack(spacing: 0) {
            Divider()

            HStack(spacing: 20) {
                // Play / Stop button
                Button {
                    engine.play()
                } label: {
                    Image(systemName: engine.isPlaying ? "pause.fill" : "play.fill")
                        .font(.title2)
                        .foregroundStyle(engine.isPlaying ? .yellow : .green)
                        .frame(width: 44, height: 44)
                        .background(Color.white.opacity(0.08))
                        .clipShape(Circle())
                }

                // Stop button
                Button {
                    engine.stop_transport()
                } label: {
                    Image(systemName: "stop.fill")
                        .font(.title3)
                        .foregroundStyle(.red)
                        .frame(width: 40, height: 40)
                        .background(Color.white.opacity(0.08))
                        .clipShape(Circle())
                }

                Divider()
                    .frame(height: 30)

                // Bar:Beat display
                VStack(spacing: 2) {
                    Text("\(engine.bar) : \(engine.beat)")
                        .font(.system(.title3, design: .monospaced).bold())
                        .foregroundStyle(.cyan)
                    Text("小节 : 拍")
                        .font(.caption2)
                        .foregroundStyle(.gray)
                }
                .frame(minWidth: 70)

                Divider()
                    .frame(height: 30)

                // Tempo slider
                VStack(spacing: 2) {
                    HStack(spacing: 4) {
                        Text("♩=")
                            .foregroundStyle(.gray)
                            .font(.caption)
                        Text("\(Int(engine.bpm))")
                            .font(.system(.headline, design: .monospaced))
                            .foregroundStyle(.white)
                    }
                    Slider(value: Binding(
                        get: { Double(engine.bpm) },
                        set: { engine.setTempo(Float($0)) }
                    ), in: 40...240, step: 1)
                    .tint(.cyan)
                    .frame(width: 160)
                }

                Spacer()

                // Audio status indicator
                HStack(spacing: 4) {
                    Circle()
                        .fill(engine.audioRunning ? Color.green : Color.red)
                        .frame(width: 8, height: 8)
                    Text("Audio")
                        .font(.caption2)
                        .foregroundStyle(.gray)
                }

                HStack(spacing: 4) {
                    Circle()
                        .fill(engine.midiConnected ? Color.cyan : Color.gray)
                        .frame(width: 8, height: 8)
                    Text("MIDI")
                        .font(.caption2)
                        .foregroundStyle(.gray)
                }
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 10)
            .background(Color(red: 0.08, green: 0.09, blue: 0.14))
        }
    }
}

#Preview {
    TransportBar().environment(EngineController.shared)
}

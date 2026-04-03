// StatusBar.swift — Top status bar: Audio + MIDI connection state
import SwiftUI

struct StatusBar: View {
    @Environment(EngineController.self) private var engine

    var body: some View {
        HStack(spacing: 16) {
            // App title
            Text("ElectoneManager")
                .font(.caption.bold())
                .foregroundStyle(.cyan)

            Spacer()

            // Audio status
            HStack(spacing: 4) {
                Circle()
                    .fill(engine.audioRunning ? .green : .red)
                    .frame(width: 7, height: 7)
                Text(engine.audioRunning
                     ? String(format: "音频 %.1fms", engine.latencyMs)
                     : "音频 离线")
                    .font(.caption2)
                    .foregroundStyle(.gray)
            }

            // MIDI status
            HStack(spacing: 4) {
                Circle()
                    .fill(engine.midiConnected ? .cyan : .gray)
                    .frame(width: 7, height: 7)
                Text(engine.midiConnected ? "MIDI 已连接" : "MIDI 未连接")
                    .font(.caption2)
                    .foregroundStyle(.gray)
            }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 6)
        .background(Color(red: 0.06, green: 0.07, blue: 0.12))
        .overlay(Divider(), alignment: .bottom)
    }
}

#Preview {
    StatusBar().environment(EngineController.shared)
}

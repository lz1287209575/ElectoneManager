// RhythmPage.swift — Tab 3: Preset selector + 12×16 drum grid
import SwiftUI

struct RhythmPage: View {
    @Environment(EngineController.self) private var engine

    var body: some View {
        VStack(spacing: 16) {
            // Header
            HStack {
                Text("节奏模式")
                    .font(.title2.bold())
                    .foregroundStyle(.white)
                Spacer()
                Button("清除", role: .destructive) {
                    engine.clearPattern()
                }
                .buttonStyle(.bordered)
                .tint(.red)
            }
            .padding(.horizontal)

            // Preset selector row
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(0..<engine.presetCount, id: \.self) { idx in
                        Button {
                            engine.selectPreset(idx)
                        } label: {
                            Text(engine.presetName(idx))
                                .font(.subheadline.bold())
                                .padding(.horizontal, 14)
                                .padding(.vertical, 8)
                                .background(engine.currentPreset == idx
                                            ? Color.cyan
                                            : Color.white.opacity(0.1))
                                .foregroundStyle(engine.currentPreset == idx
                                                 ? .black : .white)
                                .cornerRadius(20)
                        }
                    }
                }
                .padding(.horizontal)
            }

            // Pattern name
            Text(String(cString: withUnsafeBytes(of: engine.drumGrid.patternName) { ptr in
                let buffer = ptr.bindMemory(to: CChar.self)
                return String(cString: buffer.baseAddress ?? UnsafePointer(bitPattern: 0)!)
            }))
            .font(.caption)
            .foregroundStyle(.gray)

            // Drum grid
            DrumGrid()

            Spacer()
        }
        .padding(.top)
    }
}

#Preview {
    RhythmPage().environment(EngineController.shared)
}

// DrumGrid.swift — 12-row × 16-step interactive drum step sequencer grid
import SwiftUI

private let kDrumNames = [
    "BD","SD","SS","cHH","oHH","pHH","CR","RD","LT","HT","CLP","CB"
]

private let kRowColors: [Color] = [
    .red, .orange, .yellow, .green, .mint, .teal,
    .cyan, .blue, .indigo, .purple, .pink, .brown
]

struct DrumGrid: View {
    @Environment(EngineController.self) private var engine

    var body: some View {
        ScrollView([.horizontal, .vertical], showsIndicators: false) {
            VStack(spacing: 3) {
                // Step number header
                HStack(spacing: 3) {
                    // Row label placeholder
                    Text("")
                        .frame(width: 36)

                    ForEach(0..<16, id: \.self) { step in
                        Text("\(step + 1)")
                            .font(.system(size: 9, design: .monospaced))
                            .foregroundStyle(step % 4 == 0 ? .white : .gray.opacity(0.5))
                            .frame(width: 28)
                    }
                }
                .padding(.bottom, 2)

                // Drum rows
                ForEach(0..<12, id: \.self) { row in
                    DrumRow(row: row)
                }
            }
            .padding(12)
        }
        .background(Color.black.opacity(0.3))
        .cornerRadius(12)
        .padding(.horizontal)
    }
}

struct DrumRow: View {
    let row: Int
    @Environment(EngineController.self) private var engine

    var body: some View {
        HStack(spacing: 3) {
            // Row label
            Text(kDrumNames[safe: row] ?? "??")
                .font(.system(size: 10, design: .monospaced).bold())
                .foregroundStyle(kRowColors[safe: row] ?? .white)
                .frame(width: 36, alignment: .trailing)

            // 16 step cells
            ForEach(0..<16, id: \.self) { step in
                DrumCell(row: row, step: step)
            }
        }
    }
}

struct DrumCell: View {
    let row: Int
    let step: Int
    @Environment(EngineController.self) private var engine

    private var velocity: UInt8 {
        // Access grid[row][step] from the C struct
        withUnsafeBytes(of: engine.drumGrid.grid) { ptr in
            let flat = ptr.bindMemory(to: UInt8.self)
            let idx = row * 16 + step
            return idx < flat.count ? flat[idx] : 0
        }
    }

    private var isActive: Bool { velocity > 0 }
    private var isCurrent: Bool { step == engine.currentStep }
    private var accent: Bool { velocity >= 100 }

    var body: some View {
        Button {
            engine.toggleStep(row: row, step: step)
        } label: {
            RoundedRectangle(cornerRadius: 4)
                .fill(cellColor)
                .frame(width: 28, height: 28)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(isCurrent ? Color.white : Color.clear, lineWidth: 1.5)
                )
                .scaleEffect(isCurrent && engine.isPlaying ? 1.05 : 1.0)
                .animation(.spring(response: 0.1), value: isCurrent)
        }
        .buttonStyle(.plain)
    }

    private var cellColor: Color {
        if isActive {
            let base = kRowColors[safe: row] ?? .cyan
            return isCurrent ? .white : (accent ? base : base.opacity(0.65))
        } else {
            return isCurrent && engine.isPlaying
                ? Color.white.opacity(0.25)
                : Color.white.opacity(0.07)
        }
    }
}

extension Collection {
    subscript(safe index: Index) -> Element? {
        indices.contains(index) ? self[index] : nil
    }
}

// VoicePage.swift — Tab 1: 2×4 voice card grid
import SwiftUI

struct VoicePage: View {
    @Environment(EngineController.self) private var engine

    // GM program families for quick selection
    private let quickPrograms: [(name: String, gm: Int)] = [
        ("钢琴",  0),  ("弦乐",  48), ("吉他",  24), ("长笛",  73),
        ("大提琴",42), ("小提琴",40), ("合唱",  52), ("竖琴",  46),
    ]

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                Text("音色插槽")
                    .font(.title2.bold())
                    .foregroundStyle(.white)
                    .padding(.horizontal)

                LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())],
                          spacing: 12) {
                    ForEach(0..<8, id: \.self) { slot in
                        VoiceCard(slot: slot)
                    }
                }
                .padding(.horizontal)

                Divider().foregroundStyle(.gray)

                Text("快速音色切换")
                    .font(.headline)
                    .foregroundStyle(.gray)
                    .padding(.horizontal)

                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 4),
                          spacing: 8) {
                    ForEach(quickPrograms, id: \.gm) { item in
                        Button {
                            engine.setSlotProgram(slot: 0, gmProgram: item.gm)
                        } label: {
                            Text(item.name)
                                .font(.caption.bold())
                                .frame(maxWidth: .infinity)
                                .padding(.vertical, 8)
                                .background(Color.cyan.opacity(0.2))
                                .foregroundStyle(.cyan)
                                .cornerRadius(8)
                        }
                    }
                }
                .padding(.horizontal)
            }
            .padding(.vertical)
        }
    }
}

#Preview {
    VoicePage().environment(EngineController.shared)
}

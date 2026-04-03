// ContentView.swift — Root view: TabView (Voice / Score / Rhythm) + TransportBar
import SwiftUI

struct ContentView: View {
    @Environment(EngineController.self) private var engine

    var body: some View {
        VStack(spacing: 0) {
            StatusBar()

            TabView {
                VoicePage()
                    .tabItem {
                        Label("音色", systemImage: "pianokeys")
                    }

                ScorePage()
                    .tabItem {
                        Label("乐谱", systemImage: "music.note.list")
                    }

                RhythmPage()
                    .tabItem {
                        Label("节奏", systemImage: "metronome")
                    }
            }
            .tint(.cyan)

            TransportBar()
        }
        .background(Color(red: 0.06, green: 0.07, blue: 0.12))
        .preferredColorScheme(.dark)
    }
}

#Preview {
    ContentView()
        .environment(EngineController.shared)
}

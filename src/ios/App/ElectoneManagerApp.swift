// ElectoneManagerApp.swift — @main entry point
import SwiftUI

@main
struct ElectoneManagerApp: App {
    @State private var engine = EngineController.shared

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(engine)
                .onAppear { engine.start() }
                .onDisappear { engine.stop() }
        }
    }
}

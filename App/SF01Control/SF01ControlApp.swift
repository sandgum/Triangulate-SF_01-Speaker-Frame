import SwiftUI

@main
struct SF01ControlApp: App {
    @StateObject private var controller = SpeakerController()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(controller)
        }
    }
}

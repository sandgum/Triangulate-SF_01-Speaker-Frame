import SwiftUI

/// Root of the app: a bottom tab bar switching between the speaker page and
/// the equaliser. Both tabs stay alive, so each keeps its scroll position and
/// the equaliser keeps its selected band while you flip between them.
struct ContentView: View {
    var body: some View {
        TabView {
            SpeakerScreen()
                .tabItem { Label("Speaker", systemImage: "hifispeaker.fill") }

            EQScreen()
                .tabItem { Label("Equaliser", systemImage: "slider.vertical.3") }
        }
    }
}

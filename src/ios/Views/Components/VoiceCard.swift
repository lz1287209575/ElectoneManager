// VoiceCard.swift — Single voice slot card in the 2×4 grid
import SwiftUI

struct VoiceCard: View {
    let slot: Int
    @Environment(EngineController.self) private var engine
    @State private var showProgramPicker = false

    private let slotLabels = ["上排键盘", "下排键盘", "脚踏板", "主导音",
                               "打击乐", "自动贝司", "节奏1", "节奏2"]

    var body: some View {
        Button {
            showProgramPicker = true
        } label: {
            VStack(alignment: .leading, spacing: 6) {
                HStack {
                    Text("Slot \(slot + 1)")
                        .font(.caption.bold())
                        .foregroundStyle(.gray)
                    Spacer()
                    Image(systemName: "music.note")
                        .font(.caption)
                        .foregroundStyle(.cyan.opacity(0.6))
                }

                Text(slotLabels[safe: slot] ?? "---")
                    .font(.caption2)
                    .foregroundStyle(.gray.opacity(0.7))

                Spacer(minLength: 4)

                Text(engine.slotNames[safe: slot] ?? "---")
                    .font(.subheadline.bold())
                    .foregroundStyle(.white)
                    .lineLimit(2)
                    .minimumScaleFactor(0.7)

                // Layer count indicator
                let layerCount = engine.engine != nil
                    ? Int(em_voice_layer_count(engine.engine!, Int32(slot)))
                    : 0
                Text("\(layerCount) 层")
                    .font(.caption2)
                    .foregroundStyle(.cyan.opacity(0.7))
            }
            .padding(12)
            .frame(maxWidth: .infinity, minHeight: 100, alignment: .topLeading)
            .background(Color.white.opacity(0.06))
            .cornerRadius(12)
            .overlay(
                RoundedRectangle(cornerRadius: 12)
                    .stroke(Color.cyan.opacity(0.2), lineWidth: 1)
            )
        }
        .sheet(isPresented: $showProgramPicker) {
            ProgramPickerView(slot: slot)
        }
    }
}

// ─── Program picker sheet ─────────────────────────────────────────────────────
struct ProgramPickerView: View {
    let slot: Int
    @Environment(EngineController.self) private var engine
    @Environment(\.dismiss) private var dismiss

    private let gmFamilies: [(name: String, range: Range<Int>)] = [
        ("钢琴类",      0..<8),
        ("色彩打击乐",  8..<16),
        ("风琴类",     16..<24),
        ("吉他类",     24..<32),
        ("贝司类",     32..<40),
        ("弦乐类",     40..<48),
        ("合奏/合唱",  48..<56),
        ("铜管类",     56..<64),
        ("簧管类",     64..<72),
        ("笛类",       72..<80),
        ("合成旋律",   80..<88),
        ("合成铺底",   88..<96),
        ("民族乐器",  104..<112),
        ("竖琴/打击", 112..<120),
    ]

    var body: some View {
        NavigationStack {
            List {
                ForEach(gmFamilies, id: \.name) { family in
                    Section(family.name) {
                        ForEach(family.range, id: \.self) { prog in
                            Button {
                                engine.setSlotProgram(slot: slot, gmProgram: prog)
                                dismiss()
                            } label: {
                                HStack {
                                    Text("\(prog)")
                                        .font(.caption.monospaced())
                                        .foregroundStyle(.gray)
                                        .frame(width: 30)
                                    Text(gmProgramName(prog))
                                        .foregroundStyle(.white)
                                    Spacer()
                                    if engine.slotPrograms[safe: slot] == Int32(prog) {
                                        Image(systemName: "checkmark")
                                            .foregroundStyle(.cyan)
                                    }
                                }
                            }
                        }
                    }
                }
            }
            .navigationTitle("选择音色 — Slot \(slot + 1)")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("完成") { dismiss() }
                }
            }
        }
        .preferredColorScheme(.dark)
    }

    private func gmProgramName(_ prog: Int) -> String {
        let names = [
            "Acoustic Grand Piano","Bright Acoustic Piano","Electric Grand Piano",
            "Honky-tonk Piano","Electric Piano 1","Electric Piano 2","Harpsichord",
            "Clavinet","Celesta","Glockenspiel","Music Box","Vibraphone","Marimba",
            "Xylophone","Tubular Bells","Dulcimer","Drawbar Organ","Percussive Organ",
            "Rock Organ","Church Organ","Reed Organ","Accordion","Harmonica",
            "Tango Accordion","Nylon String Guitar","Steel String Guitar",
            "Electric Jazz Guitar","Electric Clean Guitar","Electric Muted Guitar",
            "Overdriven Guitar","Distortion Guitar","Guitar Harmonics",
            "Acoustic Bass","Electric Bass Finger","Electric Bass Pick",
            "Fretless Bass","Slap Bass 1","Slap Bass 2","Synth Bass 1","Synth Bass 2",
            "Violin","Viola","Cello","Contrabass","Tremolo Strings","Pizzicato Strings",
            "Orchestral Harp","Timpani","String Ensemble 1","String Ensemble 2",
            "Synth Strings 1","Synth Strings 2","Choir Aahs","Voice Oohs","Synth Voice",
            "Orchestra Hit","Trumpet","Trombone","Tuba","Muted Trumpet","French Horn",
            "Brass Section","Synth Brass 1","Synth Brass 2","Soprano Sax","Alto Sax",
            "Tenor Sax","Baritone Sax","Oboe","English Horn","Bassoon","Clarinet",
            "Piccolo","Flute","Recorder","Pan Flute","Blown Bottle","Shakuhachi",
            "Whistle","Ocarina","Lead Square","Lead Sawtooth","Lead Calliope",
            "Lead Chiff","Lead Charang","Lead Voice","Lead Fifths","Lead Bass+Lead",
            "Pad New Age","Pad Warm","Pad Polysynth","Pad Choir","Pad Bowed",
            "Pad Metallic","Pad Halo","Pad Sweep","FX Rain","FX Soundtrack",
            "FX Crystal","FX Atmosphere","FX Brightness","FX Goblins","FX Echoes",
            "FX Sci-fi","Sitar","Banjo","Shamisen","Koto","Kalimba","Bag Pipe",
            "Fiddle","Shanai","Tinkle Bell","Agogo","Steel Drums","Woodblock",
            "Taiko Drum","Melodic Tom","Synth Drum","Reverse Cymbal",
            "Guitar Fret Noise","Breath Noise","Seashore","Bird Tweet",
            "Telephone Ring","Helicopter","Applause","Gunshot"
        ]
        return (prog >= 0 && prog < names.count) ? names[prog] : "Program \(prog)"
    }
}

// ─── Safe subscript helper ─────────────────────────────────────────────────────
extension Array {
    subscript(safe index: Int) -> Element? {
        indices.contains(index) ? self[index] : nil
    }
}

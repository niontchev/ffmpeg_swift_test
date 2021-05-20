//
//  ContentView.swift
//  Shared
//
//  Created by NI on 18.05.21.
//

import SwiftUI

struct ContentView: View {
	@ObservedObject var defaultParams = UserDefaultParameters.defaults
	let audioPlayer = audio_file_player_init()
	let multiTapEffect = mt_delay_init()
	var body: some View {
		NavigationView {
			Form {
				Section(/*header: Text("Alarm")*/) {
					Picker("Source", selection: $defaultParams.selectedSound) {
						ForEach(soundFile) {sound in
							Text(sound.name)
						}
					}
					if (!defaultParams.selectedSoundDecoded) {
						Button("Decode", action: {
							if var d = FileDescriptor.getFileDescriptor(name: defaultParams.selectedSound) {
								decompressAudioFile(d.url.path);
								d.decoded = true;
								FileDescriptor.setDescriptor(name: defaultParams.selectedSound, descriptor: d)
								defaultParams.selectedSoundDecoded = true;
							}
						})
					}
					else {
						Button("Play", action: {
							if let d = FileDescriptor.getFileDescriptor(name: defaultParams.selectedSound) {
								let wavPath = (d.url.path as NSString).deletingPathExtension + ".wav"
								let success = audio_file_player_open(audioPlayer, wavPath) == 0
								if success {
									audio_file_player_start(audioPlayer)
								}
						
							}
						})
						Toggle("Enable Effect", isOn: $defaultParams.effectEnabled)
							.padding()
							.onChange(of: defaultParams.effectEnabled, perform: { (value) in
								mt_delay_set_enabled(multiTapEffect, value ? 1 : 0)
								if (value) {
									defaultParams.totalDelayMilliseconds = defaultParams.totalDelayMilliseconds == 0.0 ?
										defaultParams.totalDelayMilliseconds + 1 : defaultParams.totalDelayMilliseconds - 1;
//									mt_delay_set_wet(multiTapEffect, defaultParams.wetDry)
								}
							})
						if (defaultParams.effectEnabled) {
							HStack() {
								let maxDelay = mt_delay_get_max_delay_in_milliseconds(multiTapEffect)
								Text("Delay");
								Slider(value: $defaultParams.totalDelayMilliseconds, in: 0...maxDelay)
									.onChange(of: defaultParams.totalDelayMilliseconds, perform: { (totalDelay) in
										mt_delay_set_taps(multiTapEffect, CInt(defaultParams.numberOfTaps), totalDelay)
									})
							}
							HStack() {
								Text("Taps");
								Slider(value: $defaultParams.numberOfTaps, in: 1...16, step: 1)
									.onChange(of: defaultParams.numberOfTaps, perform: { (numberOfTaps) in
										mt_delay_set_taps(multiTapEffect, CInt(defaultParams.numberOfTaps), defaultParams.totalDelayMilliseconds)
									})
							}
							HStack() {
								Text("Mix");
								Slider(value: $defaultParams.wetDry, in: 0...1)
									.onChange(of: defaultParams.wetDry, perform: { (wetDry) in
										mt_delay_set_wet(multiTapEffect, defaultParams.wetDry)
									})
							}
						}
					}
				}
				.onChange(of: defaultParams.selectedSound, perform: { (value) in
					let _ = print(value)
				})

			}.navigationTitle("Multi-tap Delay")
		}
	}
	init() {
		let audioFilterCallback = mt_delay_get_audio_file_filter_callback()
		audio_file_player_register_filter(audioPlayer, audioFilterCallback, multiTapEffect)
		if (defaultParams.numberOfTaps == 0) { // the first time initialization
			defaultParams.numberOfTaps = Float(mt_delay_get_tap_number(multiTapEffect))
			defaultParams.wetDry = mt_delay_get_wet(multiTapEffect)
		}
		if (defaultParams.totalDelayMilliseconds == 0.0) {
			defaultParams.totalDelayMilliseconds = 200.0
		}

		mt_delay_set_enabled(multiTapEffect, defaultParams.effectEnabled ? 1 : 0)
		mt_delay_set_taps(multiTapEffect, CInt(defaultParams.numberOfTaps), defaultParams.totalDelayMilliseconds)
		mt_delay_set_wet(multiTapEffect, defaultParams.wetDry)
	}
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}

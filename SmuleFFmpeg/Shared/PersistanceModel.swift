//
//  PersistanceModel.swift
//  SmuleFFmpeg
//
//  Created by NI on 18.05.21.
//

import Foundation
import Combine
import SwiftUI

struct FileDescriptor: Identifiable {
	var url: 		URL
	var name: 		String;
	var decoded:	Bool;
	var id: 		String { name }
	static func getFileDescriptor(name: String) -> FileDescriptor? {
		return _fileDescriptorDictionary![name]
	}
	static func setDescriptor(name: String, descriptor: FileDescriptor) {
		_fileDescriptorDictionary![name] = descriptor;
	}
}

private var _fileDescriptorArray :[FileDescriptor]?
private var _fileDescriptorDictionary :[String:FileDescriptor]?

var soundFile: [FileDescriptor] {
	get {
		if (_fileDescriptorArray == nil) {
			let resourceURL = Bundle.main.resourceURL
			let fileURL = try! FileManager.default.contentsOfDirectory(at: resourceURL!, includingPropertiesForKeys: nil, options: .skipsHiddenFiles)
			let m4aFileURLArray = fileURL.filter{ $0.pathExtension == "m4a" }
			
			_fileDescriptorArray = []
			_fileDescriptorDictionary = [:]
			
			for i in 0..<m4aFileURLArray.count {
				let fileName = (m4aFileURLArray[i].lastPathComponent as NSString).deletingPathExtension
				let d = FileDescriptor(url: m4aFileURLArray[i], name: fileName, decoded: false)
				_fileDescriptorArray!.append(d)
				_fileDescriptorDictionary![fileName] = d
			}
			_fileDescriptorArray!.sort{ $0.name < $1.name }
			if UserDefaultParameters.defaults.selectedSound.count == 0 {
				UserDefaultParameters.defaults.selectedSound = _fileDescriptorArray!.count > 0 ? _fileDescriptorArray![0].name : ""
			}
		}
		return _fileDescriptorArray!
	}
}

class UserDefaultParameters : NSObject, ObservableObject {
	static var defaults : UserDefaultParameters = UserDefaultParameters()
	@Published public var selectedSound: String {
		didSet {
			UserDefaults.standard.set(selectedSound, forKey: "param.selectedSound")
			if let decrptr = FileDescriptor.getFileDescriptor(name: selectedSound) {
				selectedSoundDecoded = decrptr.decoded
			}
		}
	}
	
	@Published public var selectedSoundDecoded: Bool
	
	@Published public var effectEnabled: Bool {
		didSet {
			UserDefaults.standard.set(effectEnabled, forKey: "param.effectEnabled")
		}
	}
	@Published public var wetDry: Float {
		didSet {
			UserDefaults.standard.set(wetDry, forKey: "param.wetDry")
		}
	}
	@Published public var numberOfTaps: Float {
		didSet {
			if numberOfTaps == 0 {
				numberOfTaps = 1
			}
			UserDefaults.standard.set(numberOfTaps, forKey: "param.numberOfTaps")
		}
	}
	@Published public var totalDelayMilliseconds: Float {
		didSet {
			UserDefaults.standard.set(totalDelayMilliseconds, forKey: "param.totalDelayMilliseconds")
		}
	}

	override init() {
		//selectedSound
		selectedSound = UserDefaults.standard.string(forKey: "param.selectedSound") ?? ""
		//effectEnabled
		effectEnabled = UserDefaults.standard.bool(forKey: "param.effectEnabled")
		//wetDry
		wetDry = UserDefaults.standard.float(forKey: "param.wetDry")
		//numberOfTaps
		numberOfTaps = UserDefaults.standard.float(forKey: "param.numberOfTaps")
		//totalDelayMilliseconds
		totalDelayMilliseconds = UserDefaults.standard.float(forKey: "param.totalDelayMilliseconds")
		selectedSoundDecoded = false
		super.init()
	}
}


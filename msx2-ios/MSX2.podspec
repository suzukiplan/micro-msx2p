Pod::Spec.new do |spec|
  spec.name                 = "MSX2"
  spec.version              = "0.2.0"
  spec.summary              = "MSX2+ emulator specialized for embedded use"
  spec.homepage             = "https://github.com/suzukiplan/micro-msx2p/tree/master/msx2-ios"
  spec.license              = "MIT"
  spec.author               = { "suzukiplan" => "suzukiplan.ys@gmail.com" }
  spec.social_media_url     = "https://github.com/suzukiplan"
  spec.platform             = :ios, "12.0"
  spec.source               = { :git => "https://github.com/suzukiplan/micro-msx2p.git", :tag => "#{spec.version}" }
  spec.source_files         = "src/**/*.{c,cpp,h,hpp}", "msx2-ios/MSX2/*.{h,m,c,cpp,hpp}"
  spec.xcconfig             = { "HEADER_SEARCH_PATHS" => "msx2-ios" }
  spec.public_header_files  = "msx2-ios/MSX2/MSX2.h", "msx2-ios/MSX2/MSX2Core.h", "msx2-ios/MSX2/MSX2JoyPad.h", "MSX2/MSX2View.h"
  spec.frameworks           = "CoreAudio", "AudioToolbox"
end

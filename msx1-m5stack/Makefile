all: copy-core upload-roms upload-firmware

upload-roms:
	@echo ===== Upload ROM files to SPIFFS =====
	pio run -t uploadfs

upload-firmware:
	@echo ===== Build and Upload the firmware =====
	pio run -t upload

copy-core:
	cp ../src1/*.h include
	cp ../src1/*.hpp include

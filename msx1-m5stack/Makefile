all: copy-core upload-roms upload-firmware

upload-roms:
	@echo ===== Upload ROM files to SPIFFS =====
	pio run -t uploadfs

upload-firmware:
	@echo ===== Build and Upload the firmware =====
	pio run -t upload

copy-core:
	cp ../src/*.h include
	cp ../src/*.hpp include
	cp ../src/lz4.c src

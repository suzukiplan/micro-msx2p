all: build run

build:
	cd DLL && make
	cd MSX2Core && dotnet build
	cp DLL/libMSX2.so Test
	cp MSX2Core/bin/Debug/net5.0/MSX2Core.dll Test
	cd Test && dotnet build

run:
	cd Test && dotnet run


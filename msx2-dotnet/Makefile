all: build run

clean:
	cd DLL && make clean
	cd MSX2Core && rm -rf bin obj
	cd Test && rm -rf bin obj

build:
	cd DLL && make
	cd MSX2Core && dotnet build
	cd Test && dotnet build
	cp DLL/libMSX2.so Test/bin/Debug/net7.0

run:
	cd Test && dotnet run


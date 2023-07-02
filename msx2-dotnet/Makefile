all: build run

clean:
	cd DLL && make clean
	cd MSX2Core && rm -rf bin obj
	cd Test && rm -rf bin obj

build:
	cd DLL && make
	cd MSX2Core && dotnet build
	cd Test && dotnet build

run:
	cd Test && dotnet run


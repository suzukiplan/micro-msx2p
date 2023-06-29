all: build run

build:
	cd DLL && make
	cd MSX2Core && dotnet build
	cd RunBas && dotnet build

run:
	cd RunBas && dotnet run


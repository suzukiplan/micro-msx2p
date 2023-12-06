all: circle
	cp ../src/* .
	make -f Build.mak

circle:
	git submodule init circle
	git submodule update circle

clean:
	make -f Build.mak clean

.PHONY: build clean remake out

all: build flash console
    
build: 
		mos build --clean --local --verbose --platform=esp32

clean:
		rm -rf build deps

flash:
		mos flash

console:
		mos console




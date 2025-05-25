dir = src/
outputfilename = Application
inputfilenames = $(wildcard $(dir)*.c) $(wildcard $(dir)*.cpp) $(dir)vendor/stb_image/stb_image.cpp $(wildcard $(dir)vendor/imgui/*.cpp) $(wildcard $(dir)tests/*.cpp) $(dir)vendor/miniaudio/miniaudio_implementation.cpp
frameworks = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation
flags = -fdiagnostics-color=always -g -Wall -Iinclude -Llibrary library/libglfw.3.4.dylib $(frameworks) -Wno-deprecated -ldl -lpthread -lm -lfftw3

run: compile
	clear
	leaks -quiet -list --atExit -- ./$(outputfilename)

compile: $(dir)$(outputfilename).cpp
	g++ -std=c++11 $(inputfilenames) -o $(outputfilename) $(flags)

list:
	echo $(inputfilenames)
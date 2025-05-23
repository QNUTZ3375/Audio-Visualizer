dir = src/
outputfilename = Application
inputfilenames = $(wildcard $(dir)*.cpp) $(wildcard $(dir)*.c) $(dir)vendor/stb_image/stb_image.cpp $(wildcard $(dir)vendor/imgui/*.cpp)  $(wildcard $(dir)tests/*.cpp) $(dir)vendor/miniaudio/miniaudio_implementation.cpp
frameworks = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation

run: compile
	clear
	leaks -quiet -list --atExit -- ./$(outputfilename)

compile: $(dir)$(outputfilename).cpp
	g++ -std=c++11 $(inputfilenames) -o $(outputfilename) -fdiagnostics-color=always -g -Wall -Iinclude -Llibrary library/libglfw.3.4.dylib $(frameworks) -Wno-deprecated -ldl -lpthread -lm

list:
	echo $(inputfilenames)
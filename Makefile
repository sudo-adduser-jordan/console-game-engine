# g++ -o olcExampleProgram.exe main.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17

CC      = g++
RM      = rm -rf
VERSION = -std=c++17
EXECUTABLE = -o ConsoleGameEngine.exe
CCFLAGS = -lX11 -lGL -lpthread -lpng -lstdc++fs 
# compiler flags:
#  -g      adds debugging information to the executable file
#  -Wall   turns on most, but not all, compiler warnings
#  -Wextra turns on extra compiler warnings
# -lX11 -lGL -lpthread -lpng -lstdc++fs required for olc

default: all
all: main
main: main.cpp
	$(CC) main.cpp $(EXECUTABLE) $(CCFLAGS) $(VERSION)
	@echo "Build complete"
clean:
	$(RM) *.dSYM *.out main
	@echo "Clean complete"
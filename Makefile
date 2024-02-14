# g++ -o olcExampleProgram.exe main.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17

CC      = g++
RM      = rm -rf
VERSION = -std=c++17
CCFLAGS = -g -Wall -Wextra -lX11 -lGL -lpthread -lpng -lstdc++fs 
# EXECUTABLE = -o ConsoleGraphicsEngine.exe
# compiler flags:
#  -g      adds debugging information to the executable file
#  -Wall   turns on most, but not all, compiler warnings
#  -Wextra turns on extra compiler warnings
# -lX11 -lGL -lpthread -lpng -lstdc++fs required for olc

default: all
all: ConsoleGraphicsEngine ConsoleGraphicsEngine2
ConsoleGraphicsEngine: ConsoleGraphicsEngine.cpp
	$(CC) ConsoleGraphicsEngine.cpp -o bin/ConsoleGraphicsEngine.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
ConsoleGraphicsEngine2: ConsoleGraphicsEngine2.cpp
	$(CC) ConsoleGraphicsEngine2.cpp -o bin/ConsoleGraphicsEngine2.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
clean:
	$(RM) *.dSYM *.out ConsoleGraphicsEngine ConsoleGraphicsEngine2
	@echo "Clean complete"
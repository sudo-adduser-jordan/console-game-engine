# g++ -o olcExampleProgram.exe main.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17

CC      = g++
RM      = rm -rf
VERSION = -std=c++17


# WINDOWS
CCFLAGS = -g -Wall -Wextra  -luser32 -lgdi32 -lopengl32 -lgdiplus -lShlwapi -ldwmapi -lstdc++fs -static -std=c++17 

# LINUX
# CCFLAGS = -g -Wall -Wextra -lX11 -lGL -lpthread -lpng -lstdc++fs 

# EXECUTABLE = -o ConsoleGraphicsEngine.exe
# compiler flags:
#  -g      adds debugging information to the executable file
#  -Wall   turns on most, but not all, compiler warnings
#  -Wextra turns on extra compiler warnings
# -lX11 -lGL -lpthread -lpng -lstdc++fs required for olc

# default: all
default: ConsoleGraphicsEngine6

all: ConsoleGraphicsEngine
all: ConsoleGraphicsEngine2 
all: ConsoleGraphicsEngine3 
all: ConsoleGraphicsEngine4 
all: ConsoleGraphicsEngine5
all: ConsoleGraphicsEngine6

ConsoleGraphicsEngine: ConsoleGraphicsEngine.cpp
	$(CC) ConsoleGraphicsEngine.cpp -o bin/ConsoleGraphicsEngine.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
ConsoleGraphicsEngine2: ConsoleGraphicsEngine2.cpp
	$(CC) ConsoleGraphicsEngine2.cpp -o bin/ConsoleGraphicsEngine2.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
ConsoleGraphicsEngine3: ConsoleGraphicsEngine3.cpp
	$(CC) ConsoleGraphicsEngine3.cpp -o bin/ConsoleGraphicsEngine3.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
ConsoleGraphicsEngine4: ConsoleGraphicsEngine4.cpp
	$(CC) ConsoleGraphicsEngine4.cpp -o bin/ConsoleGraphicsEngine4.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
ConsoleGraphicsEngine5: ConsoleGraphicsEngine5.cpp
	$(CC) ConsoleGraphicsEngine5.cpp -o bin/ConsoleGraphicsEngine5.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"
ConsoleGraphicsEngine6: ConsoleGraphicsEngine6.cpp
	$(CC) ConsoleGraphicsEngine6.cpp -o bin/ConsoleGraphicsEngine6.exe $(CCFLAGS) $(VERSION)
	@echo "Build complete"

clean:
	$(RM) *.dSYM *.out \
	ConsoleGraphicsEngine \
	ConsoleGraphicsEngine2 \
	ConsoleGraphicsEngine3 \
	ConsoleGraphicsEngine4 \
	ConsoleGraphicsEngine5 \
	ConsoleGraphicsEngine6 \
	@echo "Clean complete"
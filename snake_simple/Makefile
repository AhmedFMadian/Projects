CXX      = C:/msys64/mingw64/bin/g++.exe
SDL2_CFLAGS = -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main
SDL2_LIBS   = -LC:/msys64/mingw64/lib -lmingw32 -mwindows -lSDL2main -lSDL2
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -D_USE_MATH_DEFINES $(SDL2_CFLAGS)
LDFLAGS  = $(SDL2_LIBS) -lSDL2_ttf -lSDL2_mixer -lm

TARGET   = snake
SRC      = main.cpp
HEADERS  = Constants.h Logic.h Leaderboard.h Sound.h Draw.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) scores.json

.PHONY: all run clean

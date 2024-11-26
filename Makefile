FLAGS=-O3 -I./raylib-5.5_linux_amd64/include -L./raylib-5.5_linux_amd64/lib
LIBS=-l:libraylib.a -lm
heart: heart.cpp ffmpeg_linux.cpp
	g++ -o heart $(FLAGS) heart.cpp ffmpeg_linux.cpp $(LIBS)

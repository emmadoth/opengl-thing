CC   = gcc
WARN = -Wall -Wextra -Wconversion -Wvla
COMN = -g0
LINK = -lglfw -lGLEW -lGL

default:
	$(CC) $(WARN) $(COMN) $(LINK) main.c -o test


#! /bin/sh
# --------------------------------------
#  @created: 2022-09-15
#  @author:  Douglas Vinicius
#  @email:   douglvini@gmail.com
# --------------------------------------

if not [ -d "bin" ]
then
	mkdir bin
fi

# @todo turn on warnings
cc -DDEBUG_MODE=1 -g smoking_snake.c -o bin/smoking_snake -lSDL2 -lm
#cc -DDEBUG_MODE=0 -O2 smoking_snake.c -o bin/smoking_snake -lSDL2 -lm
echo __DONE__

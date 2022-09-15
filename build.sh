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

cc -g smoking_snake.c -o bin/smoking_snake -lSDL2
echo __DONE__

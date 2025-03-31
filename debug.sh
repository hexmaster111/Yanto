rm fcon; cc -ggdb main.c -lraylib -lm -ofcon -fsanitize=address; ./fcon

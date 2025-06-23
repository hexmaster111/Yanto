#!/bin/bash
set -x
rm fcon; 
cc -ggdb edd.c -lraylib -lm -ofcon -fsanitize=address;

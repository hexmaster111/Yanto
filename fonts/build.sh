#!/bin/bash

cc fontgen.c -ggdb -lraylib -lm -ofontgen -fsanitize=address
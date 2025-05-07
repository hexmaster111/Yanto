#!/bin/bash
rm fontgen
cc fontgen.c -ggdb -lraylib -lm -ofontgen
./fontgen
#!/usr/bin/env bash

set -e

gcc -o cshell main.c -lm -Wall -Wextra

./cshell


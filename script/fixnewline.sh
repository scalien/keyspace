#!/bin/sh

find . \( -name "*.cpp" -name "*.c" -or -name "*.h" \) -exec fixnewline.py {} +
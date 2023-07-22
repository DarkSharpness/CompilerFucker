#!/bin/bash

cd src
antlr4 -visitor -no-listener -o ../gen -Dlanguage=Cpp MxLexer.g4 MxParser.g4

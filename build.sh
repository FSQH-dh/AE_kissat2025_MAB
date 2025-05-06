#!/bin/bash
./configure --competition --test  && make 2>&1
cp build/kissat ./
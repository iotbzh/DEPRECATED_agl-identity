#!/bin/bash

afb-daemon --port=9001 --token='' --binding=$PWD/build/src/afb-ll-store-binding.so --verbose

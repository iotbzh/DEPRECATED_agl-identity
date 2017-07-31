#!/bin/bash

afb-daemon --port=9000 --token='' --binding=$PWD/build/src/afb-ll-auth-binding.so --verbose --roothttp=$PWD/htdocs

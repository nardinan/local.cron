#!/bin/sh
set -ex
git clone git://github.com/nardinan/miranda
cd miranda && make && sudo make install

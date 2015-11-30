#!/bin/sh
doxygen && cd doc/latex && make
echo "Documentation generation complete"

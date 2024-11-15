#!/bin/bash

# Create the output directory if it doesn't exist
mkdir -p ./output

# Compile the client and server code
g++ -Wall -Wextra -O2 -g ./client/main.cpp -o ./output/client
g++ -Wall -Wextra -O2 -g ./server/main.cpp -o ./output/server

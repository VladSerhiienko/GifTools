
# This file is called from CMake build process,
# the working directory is here, build_emsdk/emsdk-prefix/src/emsdk-build
pwd
cd dependencies/emsdk

# Download and install the latest SDK tools.
# Make the "latest" SDK "active" for the current user. (writes ~/.emscripten file)
# Activate PATH and other environment variables in the current terminal

pwd
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

cd ../../
pwd

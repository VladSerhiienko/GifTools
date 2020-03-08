
git submodule init
git submodule update

cd dependencies/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ../../

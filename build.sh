set -e

apt update
apt install -y build-essential cmake git libtool iproute2 python3 sudo nasm libssl-dev libgmp-dev wget libfmt-dev

cd thirdparty

## Build secure-join
git clone https://github.com/Visa-Research/secure-join.git
cd secure-join
sed -i 's|set(GIT_TAG             "657f6da90bff5774a2d01c824e997572d5e8ba00")|set(GIT_TAG             "d21bc4d7aae941e276b92615252fd1760c902890")|' thirdparty/getLibOTe.cmake
python3 build.py --install -D SECUREJOIN_ENABLE_BOOST=ON
cd ..


## Build volePSI
git clone https://github.com/ladnir/volepsi.git
cd volepsi
git checkout 59e06bca81a3287257522cd261bad71e37780642
sed -i 's|36cd7242e085eddba34feaa63733ec4c6ded66c7|d21bc4d7aae941e276b92615252fd1760c902890|g' thirdparty/getLibOTe.cmake
python3 build.py --install --sudo -DVOLE_PSI_ENABLE_BOOST=true -DVOLE_PSI_ENABLE_BITPOLYMUL=false -DVOLE_PSI_SODIUM_MONTGOMERY=false
cp ./out/build/linux/volePSI/config.h /usr/local/include/volePSI/
cd ..

cd ..

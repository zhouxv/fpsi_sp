cd thirdparty/secure-join
mkdir -p out && curl -L "https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.bz2" | tar -xvj -C out
python3 build.py --install --sudo -DSECUREJOIN_ENABLE_BOOST=ON ## replace the url of boost with https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.bz2
# replace setBytes

cd ../../
mkdir -p build && cd build
cmake .. && make -j


git clone https://github.com/ladnir/volepsi.git
git checkout 59e06bca81a3287257522cd261bad71e37780642
# set(GIT_TAG             "e12bfbb459ad38908e110a9db538f2b82fb14d18" )
python3 build.py --install --sudo -DVOLE_PSI_ENABLE_BOOST=true -DVOLE_PSI_ENABLE_BITPOLYMUL=false -DVOLE_PSI_SODIUM_MONTGOMERY=false

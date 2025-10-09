cd thirdparty/secure-join
mkdir -p out && curl -L "https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.bz2" | tar -xvj -C out
python3 build.py --install --sudo -DSECUREJOIN_ENABLE_BOOST=ON ## replace the url of boost with https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.bz2

cd ../../
mkdir -p build && cd build
cmake .. && make -j

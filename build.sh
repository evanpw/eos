set -e
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Standard ..
cmake --build . -j8
cd -

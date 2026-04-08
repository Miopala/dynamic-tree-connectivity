
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)

python3 scripts/validate.py
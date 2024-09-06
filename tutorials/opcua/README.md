# Clone the repository 
git clone https://github.com/open62541/open62541.git
cd open62541
git checkout 46d0395
git submodule update --init --recursive

# Copy modified server_ctt.c which is configured for anonymous user authentication 
cp ../server_ctt.c examples

# Build open62541 with examples
AFL_HARDEN=1 cmake -DCMAKE_C_COMPILER=afl-clang-fast -DCMAKE_CXX_COMPILER=afl-clang-fast++ -DBUILD_SHARED_LIBS=ON -DUA_BUILD_EXAMPLES=ON DUA_ENABLE_ENCRYPTION=OFF ../open62541
make -j1
make install

# Copy the server
cd ..
cp open62541/bin/examples/server_ctt .

# Fuzzing
afl-fuzz -i in-opcua -o out/<name> -N tcp://127.0.0.1/4840 -P OPCUA -q 3 -s 3 -E -K -R ./server_ctt

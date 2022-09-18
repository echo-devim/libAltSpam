#Linux
echo "Compiling for Linux..."
g++ -O2 main.cpp lib/*.cpp -o altspam

#Windows
echo "Compiling for Windows (MinGW required)..."
i686-w64-mingw32-c++ -O2 main.cpp lib/*.cpp -o altspam.exe -lws2_32

#Shared Object
echo "Compiling library (.so)..."
g++ -shared -fPIC -o libAltSpam.so main.cpp lib/*.cpp

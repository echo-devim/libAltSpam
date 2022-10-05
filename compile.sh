#Linux
echo "Compiling for Linux..."
g++ -O2 main.cpp lib/*.cpp misc/*.cpp imap/*.cpp -o altspam -lcurl

#Windows
echo "Compiling for Windows (MinGW required)..."
i686-w64-mingw32-c++ -O2 -I ../dependencies/curl-7.85.0_4-win32-mingw/include main.cpp lib/*.cpp misc/*.cpp imap/*.cpp -o altspam.exe -lws2_32 ../dependencies/curl-7.85.0_4-win32-mingw/lib/libcurl.dll.a -DCURL_STATICLIB

#Shared Object
echo "Compiling library (.so)..."
g++ -shared -fPIC -o libAltSpam.so main.cpp lib/*.cpp misc/*.cpp

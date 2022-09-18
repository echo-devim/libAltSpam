# AltSpam

This repository contains the source code of libAltSpam, a lightweight and portable antispam library written in C++. Included in this repo there are also a command line interface tool and a tiny API server (single client).

The idea of this project is to check if an input email is spam or not assigning it a score. The score is assigned using several checks on the headers, the charsets, url domains and "spam words".
The library **is not** based on any AI algorithm, because it must be lightweight. You can mark emails as spam in order to add the words of those email to the spam word count database.

Check `https://github.com/echo-devim/AltSpam` for a complete antispam service with graphical user interface based on this library.

## Details
The core of the library is the word count function. Each time you give in input an email marked as spam, the library parses the email and then adds the words in the email to a global word count database called `wc.dat`.
This is a text file created in the program directory that simply contains a word count.

Example:
```
test 5
hello 1
click 8
here 8
```

The first column contains the words, the second column contains the count.
Each time you add an email to spam, the library updates the word count database adding the new words or increasing the existing value. In the same way, when you mark an email as not spam, the library updates the word count database decreasing the words or removing them if the count is less than 1.

After a first setup where you mark spam emails to populate the word count, the antispam is ready.

## Code Structure
Emails are strings that can be parsed and represented by Email class. The Email class is a basic email object representation. There is also the EnrichedEmail class that extends Email object with more details (e.g. the list of links contained in the body).
Each Email object can contain one or more links represented by Link class.
Finally, there is the AltSpam class that implements all the checks on a EnrichedEmail object to calculate a spam score for it.

## API
This repo contains a tiny API server implementation. The default port is 22525/tcp and it listens on localhost interface. It is not recommended to expose the service on the network. 

The messages accepted by the server must have the following structure:
```
|OPERATION|SIZE|DATA|
```
*  OPERATION is one byte representing the function of AltSpam to call (check `AltSpam.h` for details)
*  SIZE is an integer representing the data length
*  DATA is the argument of OPERATION

Note: Messages are protected by a very basic XOR encryption.

## CLI
This repo contains a CLI tool which allows to call the library functions.

```
usage: ./altspam [options] ... 
options:
  -l, --loadwc              load word vector from path (string [=])
  -s, --savewc              save word vector to path (string [=])
      --importwc            import word vector from existing file (string [=])
  -t, --threshold           set spam threshold (float [=0.9])
  -a, --add-to-spam         add an email to spam dataset (string [=])
  -r, --remove-from-spam    remove an email from spam dataset (string [=])
  -i, --is-spam             check if an email is spam or not (string [=])
      --spam-score          get the spam score for the specified email (string [=])
  -v, --verbose             show debug prints
      --server              start API server
  -?, --help                print this message
```

Example of usage:
```sh
$ ./altspam --server
```

## Compilation

Just run the `compile.sh` included in this repo or use the following commands.

Linux:
```sh
$ g++ -O2 main.cpp lib/*.cpp -o altspam
```

Cross-compilation for Windows (32 bit):
```sh
i686-w64-mingw32-c++ -O2 main.cpp lib/*.cpp -o altspam.exe -lws2_32
```

Library (shared object):
```sh
g++ -shared -fPIC -o libAltSpam.so main.cpp lib/*.cpp
```
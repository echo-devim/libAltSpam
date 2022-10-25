# AltSpam

This repository contains the source code of libAltSpam, a lightweight and portable antispam library written in C++. Included in this repo there are also a command line interface tool and a tiny API server (single client).

The idea of this project is to check if an input email is spam or not assigning it a score. The score is assigned using several (offline) checks on the headers, the charsets, url domains and "spam words".
The library **is not** based on any AI algorithm, because it must be lightweight. You can mark emails as spam in order to add the words of those email to the spam word count database.

Check [AltSpam Java App](https://github.com/echo-devim/JAltSpam) for a complete antispam service with graphical user interface based on this library.

## Details
The source code of the library is contained in the `lib` directory of this repo. The core of the library is the word count function. Each time you give in input an email marked as spam, the library parses the email and then adds the words in the email to a global word count database called `wc.dat`.
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
In the `imap` directory there is a limited imap client implementation based on libcurl that can retrieve emails from a remote server.
The `misc` directory contains utility functions like cryptographic functions.

## API
This repo contains a tiny API server implementation for AltSpam Remote Procedure Call (RPC). The default port is 22525/tcp and it listens on localhost interface. It is not recommended to expose the service on the network. 

The messages accepted by the server must have the following structure:
```
|OPERATION|SIZE|DATA|
```
*  OPERATION is one byte representing the function of AltSpam to call (check `AltSpam.h` for details)
*  SIZE is an integer representing the data length
*  DATA is the argument of OPERATION

Note: Messages are protected by a very basic XOR encryption.

The server exposes only the functions offered by the library, thus for example imap client functions are not available.

## CLI
This repo contains a CLI tool which allows to call the library functions, use the imap client and start the server API.

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
      --list                get the latest 10 email for each folder and print their subject to stdout
      --list-score          get the latest 10 email for each folder and print their scores to stdout
  -m, --monitor             monitor the latest 10 email and move to junk spam emails each n minutes (int [=10])
      --spam-folder         set spam folder name where to move spam messages (string [=JUNK])
      --imap-conf           imap configuration file (string [=])
      --init-imap           interactive imap configuration (string [=prop.ini])
  -v, --verbose             show debug prints
  -d, --delete              when combined with monitor option, force spam emails deletion
      --server              start API server
  -?, --help                print this message

```

### Examples

Start the (single-client) local server API:

```sh
$ ./altspam --server
```
Now the binary is listening on 22525/tcp ready for a client.

Configure your IMAP connection using the interactive setup:
```sh
$ ./altspam --init-imap
```

This command will create a new file with connection properties such as (email, encrypted password, server, folders to monitor, etc.).

List last 10 received unseen emails for each folder and print their scores:
```sh
$ ./altspam --imap-conf conn.dat --list-score
```

Example of output:
```
ID	FOLDER	SUBJECT	SPAM	SCORE
1	INBOX	test	0	0.292308
2	INBOX	test2	0	0.325428
3	INBOX	test_spam123	1	1.341849
...
```

Now, we can add or remove an email to/from the dataset using the ID.
For example let's add the test2 email to spam:
```sh
./altspam --imap-conf conn.dat -a 2
```

You can monitor your inbox folders for (unread) spam messages each 10 minutes on a remote IMAP server with the following command:
```sh
$ ./altspam --imap-conf conn.dat --monitor 10
```

Note: if you use 0 as value for `--monitor`, the application checks for spam messages only once and then it'll exit.

Now, you are ready to easily create an antispam service in your system

## Compilation

Just run the `compile.sh` included in this repo to cross-compile the project against many platforms or to generate the library (shared object).
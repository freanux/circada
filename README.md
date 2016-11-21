# Circada IRC client

[![travis](https://img.shields.io/travis/freanux/circada.svg)](https://travis-ci.org/freanux/circada)
[![license](https://img.shields.io/github/license/freanux/circada.svg)](https://github.com/freanux/circada/blob/master/COPYING)

I work often in console or in a SSH session. So I have to use an irc console client. For years I used Irssi, then I tried out WeeChat for a while. Then I switched back to Irssi.

WeeChat is too fancy for me, I don’t like to turn off all these bling bling colors before I can work with. I like Irssi very much, but it lacks of the nicklist on the right side, or the overview of all connected servers with its opened channels on the left.

So I decided to write my own IRC client completely from scratch in C++. It’s a fine mixture of Irssi and WeeChat:

![alt tag](https://raw.githubusercontent.com/freanux/circada/master/pictures/circada1.png)

## Circada features
* multiple windows
* navigation bar to easily change between windows
* scrollable nicklist
* scrollable window list with noise indicator
* SSL/TLS support
* DDC chat and file transfer
* Circada can be compiled in shared library mode to create the shared Circada library. With that you can easily write your own client or your pretty IRC bot.
* TAB completion

## Circada modes
It comes with two modes, normal chat mode and the navigation mode. To switch between the modes, simply press the ESCAPE key.

In the chat mode, to submit a chat line press ENTER. The usual IRC commands can be entered with a leading slash (/), like /nick or /me. Tab completion helps to find the correct nick.

In the navigation mode, a window bar appears. Press the arrow keys to slide between the windows, or, enter the destination window number to switch to. In this mode, following shortcuts are integrated:

* H – toggles the highlight window. This window collects all messages, which are forwarded to you.
* D – deletes the entries in the highlight window of the current screen.
* A – clears all messages in the hightlight window.
* C – toggles the channellist window on the left side.
* N – toggles the nicklist window on the right side.
* P – purges the currently selected mru entry. So you can delete some login password commands to the NickServ service.

![alt tag](https://raw.githubusercontent.com/freanux/circada/master/pictures/circada2.png)

## Installation without shared Circada library
Download Circada from this web page or clone the bleeding edge from GitHub. Then untar the tarball. Change into your untarred circada project directory. Now you have to setup your compile process:

```
$ autoreconf -i
$ ./configure
$ make -j`nproc`
```

install Circada:

```
$ sudo make install-strip
```

That's it!

## Installation with shared Circada library
Download Circada from this web page or clone the bleeding edge from GitHub. Then untar the tarball. Change into your untarred circada project directory. Now you have to setup your compile process:

```
$ autoreconf -i
$ ./configure --enable-shared-library
$ make -j`nproc`
```

Install Circada with rebuilding your ldcache:

```
$ sudo make install-strip
$ sudo ldconfig
```

Your library is now installed in eg. /usr/local/lib. Your header files are installed in eg. /usr/local/include/Circada. To use Circada in your project, include Circada like that:

```
#include <Circada/Circada.hpp>

using namespace Circada;

int main(int argc, char *argv[]) {
    .
    .
    .

    return 0;
}
```

## You need help?
Circada help channel is hosted on freenode's channel #circada.

## You want to contribute to Circada?
If you are familiar with the IRC protocol and you have well-engineered knowledge in C++, or you want to help us writing help pages or supporting us in some other way, we appriciate your effort. Contact us on Circada's freenode channel and ask there.

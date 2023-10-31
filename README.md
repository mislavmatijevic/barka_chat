# barka_chat
C++ low-level implementation of a client-server application I made in October 2020.

## About

Back when I created this solution, I was really into C++. So I played around with POSIX threads and low-level binding to sockets. After fooling around, I ended up with an actual chat client and server which together work perfectly.

User can join a server, identify with a username, discover other active users, pick a user to chat with, get notified when another user joins in or leaves, and leave the server without the server crashing (laugh all you want, but back when I programmed this, the major problem was to not crash the server on any exit by user).
The problem was also how to send messages, so there is an entire protocol set up for negotiating and exchanging messages.

## Setting up
In order to run the application, you need to be on a Linux system and have the [ncurses](https://invisible-island.net/ncurses/) library ready to be linked.
Compile the application with:

```g++ -g chat_klijent.cpp -lpthread -lncursesw -o chat_klijent```

Given makefile statically links the ncurses library in the output file allowing you to run this piece of cool little software on any Linux system.

In order to compile chat_server, you need a -lpthread flag during compilation.

## Screenshots of how does the app look like
Everything in the UI is in Croatian. Back when I did this, I did it for my soul, not to show off to the world.

Entering user's name.
![entering username](https://github.com/mislavmatijevic/barka_chat/assets/11699688/e103bcc8-2125-45ba-8ed3-b806d36ddf0a)

Left: server logging new user's join (Mislav). Right: client's UI.
![slika](https://github.com/mislavmatijevic/barka_chat/assets/11699688/75a9cfa4-bc26-467e-9e3c-a73aa0ec7e4d)

Left: server logging another user's join (Ivan). Upper-right: original user's UI, which states another user has joined. Lower-right: new user's UI, empty
![two clients UI](https://github.com/mislavmatijevic/barka_chat/assets/11699688/ccc3f04f-3bcf-4266-b626-c33cd2e3c27c)

Ivan enters '?s' to look at available users. He is given a choice to pick the user he wants to chat with:
![list of available users](https://github.com/mislavmatijevic/barka_chat/assets/11699688/3e7989bc-21b5-432f-b0c4-19b95147048b)

Example of chatting, with a server view as well. It's visible that UTF-8 characters čćšđž are correctly parsed by the server, but Windows CMD I was unfortunately using instead of a normal Linux Terminal failed to display these chars properly in the client app.
![chit-chat](https://github.com/mislavmatijevic/barka_chat/assets/11699688/8b364bc4-f317-405b-a454-23b8769f76dd)

Example of Ivan leaving the chat gracefully.
![leaving gracefully](https://github.com/mislavmatijevic/barka_chat/assets/11699688/de49f577-7eb5-45a0-ac72-d6e386d87514)

Example of Ivan closing the client process with "kill".
![leaving with kill](https://github.com/mislavmatijevic/barka_chat/assets/11699688/ab65a31c-e154-4e87-ba8a-b2873e0ae06f)

Example of a long chat and an overfill which results in overwritting older messages from the top.
![lot of messages](https://github.com/mislavmatijevic/barka_chat/assets/11699688/d076b481-c358-42b2-857e-8ddd8ddf7b38)

Client noticing window too small.
![window is too small error](https://github.com/mislavmatijevic/barka_chat/assets/11699688/3424ca49-c8d4-44d3-a3de-02058ad96b28)

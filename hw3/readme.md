# Homework 3: a small shell

A UNIX shell. 

Supports commands with max length of 2048 characters and 512 arguments
Supports '#' for commenting
Supports redirection with '<' and '>'
Supports running a command in the background with '&'
Catches signals, including SIGINT, and SIGSTP
Supports the built-in commands `exit`, `cd`, and `status`
Does not support quoting (so no spaces in arguments), or the '|' operator
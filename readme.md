# Operating Systems:

This repo contains coursework from my operating systems class (my proudest A grade!)

## Homework 1:
Use bash to perform matrix math on matrices given via stdin
Used to get familiar with bash, and stdin/stdout/stderr

## Homework 2:
A simple game
The `buildrooms` file generates 6 random rooms, and builds the graph of connections between them
The `adventure` file walks the user through navigating the rooms from starting node to ending node
Built in c99

## Homework 3:
A UNIX shell. 

Supports commands with max length of 2048 characters and 512 arguments
Supports '#' for commenting
Supports redirection with '<' and '>'
Supports running a command in the background with '&'
Catches signals, including SIGINT, and SIGSTP
Supports the built-in commands `exit`, `cd`, and `status`
Does not support quoting (so no spaces in arguments), or the '|' operator

## Homework 4:
A one-time pad style encryption + decryption service

Keygen: creates a key of a given length.
`./keygen 128` will create a 128 character key

otp_enc_d: a daemon that receives a connection from a client. It receives a key and a plaintext file, encrypts the plaintext using the key, and sends the encrypted message back to the client.
`./otp_enc_d <port to listen on>`

otp_enc: given a key, and a plaintext file, send the file to be encypyed by a daemon
`./otp_enc <plaintext> <key> <port>`

otp_dec_d: a daemon that receives a connection from a client. It receives a key, and a ciphertext file, decrypts the ciphertext using the key, and sends the decrypted message back to the client. 
`./otp_dec_d <port to listen on>`

otp_dec: given a key, and a ciphertext file, send the file to be decrypted by a daemon
`./otp_dec <ciphertext> <key> <port>`
Please use the `compileall.sh` script to compile each file
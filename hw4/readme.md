# Homework 4: one-time pad encryption + decryption service

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

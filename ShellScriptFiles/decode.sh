#!/usr/bin/env bash

#echo $1 | openssl enc -d -base64 
#echo $1 | openssl enc -d -des3 -base64 -pass pass:mypassword -pbkdf2 
echo $1 | openssl aes-256-cbc -d -a -pass pass:somepassword 
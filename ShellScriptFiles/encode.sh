#!/usr/bin/env bash

#echo $1 | openssl enc -base64 
#echo $1 | openssl enc -e -des3 -base64 -pass pass:mypassword -pbkdf2

echo $1 | openssl aes-256-cbc -a -salt -pass pass:somepassword
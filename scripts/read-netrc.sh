#!/usr/bin/bash

# fot the love of me I could not get ncftp to use .netrc directly...
ftp_username=$( cat ~/.netrc | grep login | sed "s/\\w*\\s//g" )
ftp_password=$( cat ~/.netrc | grep password | sed "s/\\w*\\s//g" )

function ftpup {
    ncftpput -m -u $ftp_username -p $ftp_password ftp.mpacula.com $1 $2
}

#!/usr/bin/env bash

###
 # RepoUpdater.sh
 # A script to sync the NextLoader and NextLoader-UDK Repos with upstream
 #
 # Copyright (c) 2020-2021 Dayo Akanji
 # MIT License
###

# Provide custom colours
msg_base() {
    echo -e "\033[0;36m$1\033[0m"
}
msg_info() {
    echo -e "\033[0;33m$1\033[0m"
}
msg_status() {
    echo -e "\033[0;32m$1\033[0m"
}
msg_error() {
    echo -e "\033[0;31m$1\033[0m"
}

## ERROR HANDLER ##
runErr() { # $1: message
    # Declare Local Variables
    local errMessage

    errMessage="${1:-Runtime Error ... Exiting}"
    echo ''
    msg_error "${errMessage}"
    echo ''
    echo ''
    exit 1
}
trap runErr ERR


## UPDATE MAIN BRANCH ##
clear
msg_info '## RepoUpdater ##'
msg_info '-----------------'
echo ''
msg_base "Syncing NextLoader-UDK"
BASE_DIR="${HOME}/Documents/NextLoader/edk2"
# shellcheck disable=SC1090
source "${BASE_DIR}/000-BuildScript/RepoUpdateSHA.txt"
pushd ${BASE_DIR} > /dev/null || runErr "ERROR: Could not find ${BASE_DIR} ...Exiting"
git checkout main
git reset --hard "${NEXTLOADER_UDK_SHA}"
git push origin HEAD -f
git pull --tags upstream main
git push origin
git push --tags origin
popd > /dev/null || runErr "ERROR: Could not return to starting directory ...Exiting"
echo ''
msg_status "Synced NextLoader-UDK"
echo ''
echo ''


## UPDATE NextLoader BRANCH ##
msg_base "Syncing NextLoader"
BASE_DIR="${HOME}/Documents/NextLoader/Working"
pushd ${BASE_DIR} > /dev/null || runErr "ERROR: Could not find ${BASE_DIR} ...Exiting"
git checkout master
git reset --hard "${NEXTLOADER_SHA}"
git push origin HEAD -f
git pull --tags upstream NextLoader
git push origin
git push --tags origin
popd > /dev/null || exit 1
echo ''
msg_status "Synced NextLoader"
echo ''
msg_info '-----------------'
msg_info '## RepoUpdater ##'
echo ''
echo ''

#!/usr/bin/env bash

###
 # rEFInd.sh
 # A script to build rEFInd
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


# Set things up for build
clear
msg_info '## rEFIndBuilder - Setting Up ##'
msg_info '------------------------------------'
sleep 2
EDIT_BRANCH="${1:-main}"
BASE_DIR="${HOME}/Documents/rEFInd"
WORK_DIR="${BASE_DIR}/Working"
EDK2_DIR="${BASE_DIR}/edk2"
if [ ! -d "${EDK2_DIR}" ] ; then
    msg_error "ERROR: Could not locate ${EDK2_DIR}"
    echo ''
    exit 1
fi
XCODE_DIR_REL="${EDK2_DIR}/Build/rEFInd/RELEASE_XCODE5"
XCODE_DIR_DBG="${EDK2_DIR}/Build/rEFInd/DEBUG_XCODE5"
XCODE_DIR_TMP="${EDK2_DIR}/.Build-TMP/rEFInd/RELEASE_XCODE5"
BINARY_DIR="${XCODE_DIR_REL}/X64"
OUTPUT_DIR="${EDK2_DIR}/000-BOOTx64-Files"
GLOBAL_FILE="${EDK2_DIR}/rEFIndPkg/EfiLib/globalExtra.h"
GLOBAL_FILE_TMP_REL="${EDK2_DIR}/rEFIndPkg/EfiLib/globalExtra-REL.txt"
GLOBAL_FILE_TMP_DBG="${EDK2_DIR}/rEFIndPkg/EfiLib/globalExtra-DBG.txt"

pushd ${WORK_DIR} > /dev/null || exit 1
msg_base "Checkout '${EDIT_BRANCH}' branch..."
git checkout ${EDIT_BRANCH} > /dev/null
msg_status '...OK'; echo ''
sleep 2
msg_info "Update rEFIndPkg..."

# Remove later #
rm -fr "${EDK2_DIR}/rEFIndPkg"
# Remove later #

rm -fr "${EDK2_DIR}/rEFIndPkg"
cp -fa "${WORK_DIR}" "${EDK2_DIR}/rEFIndPkg"
rm -fr "${EDK2_DIR}/rEFIndPkg/.gitignore"
rm -fr "${EDK2_DIR}/rEFIndPkg/.git"
msg_status '...OK'; echo ''
sleep 2
popd > /dev/null || exit 1


# Basic clean up
clear
msg_info '## rEFIndBuilder - Initial Clean Up ##'
msg_info '------------------------------------------'
sleep 2


# Remove later #
if [ -d "${EDK2_DIR}/Build-DBG" ] ; then
    rm -fr "${EDK2_DIR}/Build-DBG"
fi
if [ -d "${EDK2_DIR}/Build-TMP" ] ; then
    rm -fr "${EDK2_DIR}/Build-TMP"
fi
if [ -d "${EDK2_DIR}/Build-OLD" ] ; then
    rm -fr "${EDK2_DIR}/Build-OLD"
fi
if [ -d "${EDK2_DIR}/rEFIndPkg-OLD" ] ; then
    rm -fr "${EDK2_DIR}/rEFIndPkg-OLD"
fi
# Remove later #


if [ -d "${EDK2_DIR}/Build" ] ; then
    rm -fr "${EDK2_DIR}/Build"
fi
mkdir -p "${EDK2_DIR}/Build"
if [ -d "${OUTPUT_DIR}" ] ; then
    rm -fr "${OUTPUT_DIR}"
fi
mkdir -p "${OUTPUT_DIR}"


# Build release version
clear
msg_info '## rEFIndBuilder - Building REL Version ##'
msg_info '----------------------------------------------'
sleep 2
pushd ${EDK2_DIR} > /dev/null || exit 1
if [ -d "${EDK2_DIR}/.Build-TMP" ] ; then
    rm -fr "${EDK2_DIR}/.Build-TMP"
fi
if [ -f "${GLOBAL_FILE}" ] ; then
    rm -fr "${GLOBAL_FILE}"
fi
cp "${GLOBAL_FILE_TMP_REL}" "${GLOBAL_FILE}"
source edksetup.sh BaseTools
build
if [ -d "${EDK2_DIR}/Build" ] ; then
    cp "${BINARY_DIR}/rEFInd.efi" "${OUTPUT_DIR}/BOOTx64-REL.efi"
    mv "${EDK2_DIR}/Build" "${EDK2_DIR}/.Build-TMP"
fi
popd > /dev/null || exit 1
echo ''
msg_info "Completed REL Build on '${EDIT_BRANCH}' Branch of rEFInd"
sleep 2
msg_info "Preparing DBG Build..."
echo ''
sleep 4


# Build debug version
clear
msg_info '## rEFIndBuilder - Building DBG Version ##'
msg_info '----------------------------------------------'
sleep 2
pushd ${EDK2_DIR} > /dev/null || exit 1
if [ -f "${GLOBAL_FILE}" ] ; then
    rm -fr "${GLOBAL_FILE}"
fi
cp "${GLOBAL_FILE_TMP_DBG}" "${GLOBAL_FILE}"
source edksetup.sh BaseTools
build
if [ -d "${EDK2_DIR}/Build" ] ; then
    cp "${BINARY_DIR}/rEFInd.efi" "${OUTPUT_DIR}/BOOTx64-DBG.efi"
    mv "${XCODE_DIR_REL}" "${XCODE_DIR_DBG}"
    mv "${XCODE_DIR_TMP}" "${XCODE_DIR_REL}"
fi
if [ -d "${EDK2_DIR}/.Build-TMP" ] ; then
    rm -fr "${EDK2_DIR}/.Build-TMP"
fi
popd > /dev/null || exit 1
echo ''
msg_info "Completed DBG Build on '${EDIT_BRANCH}' Branch of rEFInd"
echo ''


# Tidy up
if [ -f "${GLOBAL_FILE}" ] ; then
    rm -fr "${GLOBAL_FILE}"
fi
cp "${GLOBAL_FILE_TMP_REL}" "${GLOBAL_FILE}"
echo ''
msg_status "rEFInd EFI Files (BOOTx64)      : '${OUTPUT_DIR}'"
msg_status "rEFInd EFI Files (Others - DBG) : '${XCODE_DIR_DBG}/X64'"
msg_status "rEFInd EFI Files (Others - REL) : '${XCODE_DIR_REL}/X64'"
echo ''
echo ''

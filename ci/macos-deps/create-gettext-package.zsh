#!/bin/zsh

# 1. Install wget
# 2. Put the script to an empty folder.
# 3. See examples below: 
#    - For arm64 architecture run: ./create-gettext-package.zsh --gettext-version=0.21.1 --architecture=arm64
#    - For x86_64 architecture run: ./create-gettext-package.zsh --gettext-version=0.21.1 --architecture=x86_64
#    - To build the universal binaries, run: ./create-gettext-package.zsh --gettext-version=0.21.1 --architecture=universal

download_gettext() {
    if [ ! -f "${PACKED_SRC_FILENAME}" ]
    then
        echo "### Downloading the gettext sources..."
        # https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz
        wget ${PACKED_SRC_URL}
        if [ $? -ne 0 ]
        then
            echo "### Could not fetch the gettext sources!"
            exit 1
        fi
    else
        echo "### ${PACKED_SRC_FILENAME} exits. Please remove it if you want to re-download it."
    fi

    if [ -d "${SRC_FOLDER}" ]; then
        echo "### ${SRC_FOLDER} folder exits. Please remove it if you want to unpack it from scratch."
        return
    fi

    echo "### Unpacking the gettext sources..."

    mkdir "${ARCHITECTURE}"
    tar -xf "${PACKED_SRC_FILENAME}" -C "${ARCHITECTURE}"
    if [ $? -ne 0 ]
    then
        echo "### Could not unpack gettext sources."
        exit 1
    fi 
}

build_gettext() {
    if [ ! -d "${SRC_FOLDER}" ]
    then
        echo "### The sources ${SRC_FOLDER} folder does not exit. Could not configure to build!"
        exit 1
    fi

    if [ -d "${PACKAGE_FOLDER}" ]
    then
        echo "### The package ${PACKAGE_FOLDER} folder exits. The build process will be skipped. Remove the folder and start the script again if you want a fresh build."
        return
    fi

    cd "${SRC_FOLDER}"

    echo "### Configuring..."

    case "${ARCHITECTURE}" in
        arm64)            
            ./configure --host=aarch64-apple-darwin --disable-shared --enable-static CFLAGS="-mmacosx-version-min=11.0 -O3 -DNDEBUG" --prefix="${PACKAGE_FOLDER}"
            ;;
        x86_64)
            ./configure --host=x86_64-apple-darwin --disable-shared --enable-static CFLAGS="-mmacosx-version-min=10.15 -O3 -DNDEBUG" --prefix="${PACKAGE_FOLDER}"
            ;;
        universal)
            ./configure --disable-shared --enable-static CFLAGS="-arch x86_64 -arch arm64 -mmacosx-version-min=10.15 -O3 -DNDEBUG" LDFLAGS="-arch x86_64 -arch arm64" --prefix="${PACKAGE_FOLDER}"
            ;;
        *)
            echo "Unknown architecture! Only 'arm64', 'x86_64' or 'universal' is supported."
            exit 1
            ;;
    esac

    if [ $? -ne 0 ]
    then
        echo "### 'configure' failed!"
        cd ${INITIAL_WORKING_FOLDER}
        exit 1
    fi

    echo "### Building..."

    make
    if [ $? -ne 0 ]
    then
        echo "### 'make' failed!'"
        cd ${INITIAL_WORKING_FOLDER}
        exit 1
    fi

    echo "### Collecting files for the package..."

    make install
    if [ $? -ne 0 ]
    then
        echo "### 'make install' failed!'"
        cd ${INITIAL_WORKING_FOLDER}
        exit 1
    fi

    cd ${INITIAL_WORKING_FOLDER}
}

package_gettext() {
    if [ ! -d "${PACKAGE_FOLDER}" ]
    then
        echo "### The package folder ${PACKAGE_FOLDER} was not found!"
        exit 1
    fi
    
    if [ -f "${PACKAGE_ZIP}" ]
    then
        echo "### The package ${PACKAGE_ZIP} exists! Please remove it and start the script again."
        exit 1
    fi    

    cp "${SCRIPT_PATH}" "${PACKAGE_FOLDER}"

    echo "### Compressing ..."
    zip --quiet --recurse-paths "${PACKAGE_ZIP}" "${PACKAGE_FOLDER_NAME}"
    if [ $? -ne 0 ]
    then
        echo "### Could not compress!"
        exit 1
    fi 

    echo "### Done: ${PACKAGE_ZIP}"
}

# SCRIPT START

ARCHITECTURE=$(uname -m)
GETTEXT_VERSION=0.21.1

while [ $# -gt 0 ]; do
    case "$1" in
        --gettext-version*)
            GETTEXT_VERSION="${1#*=}"
            ;;
        --architecture=*)
            ARCHITECTURE="${1#*=}"
            ;;
        *)
            echo "### Invalid command line parameters. Please use --gettext-version=... --architecture=\"arm64|x86_64|universal\"."
            exit 1
    esac
    shift
done

INITIAL_WORKING_FOLDER=${PWD}
SCRIPT_PATH=$(realpath $0)
PACKED_SRC_FILENAME=gettext-${GETTEXT_VERSION}.tar.gz
PACKED_SRC_URL=https://ftp.gnu.org/pub/gnu/gettext/${PACKED_SRC_FILENAME}
SRC_FOLDER_NAME=gettext-${GETTEXT_VERSION}
SRC_FOLDER=${INITIAL_WORKING_FOLDER}/${ARCHITECTURE}/${SRC_FOLDER_NAME}
PACKAGE_FOLDER_NAME=gettext-${GETTEXT_VERSION}-osx-${ARCHITECTURE}
PACKAGE_FOLDER=${INITIAL_WORKING_FOLDER}/${PACKAGE_FOLDER_NAME}
PACKAGE_ZIP_NAME=${PACKAGE_FOLDER_NAME}.zip
PACKAGE_ZIP=${INITIAL_WORKING_FOLDER}/${PACKAGE_ZIP_NAME}

# Check if wget is available
if ! command -v git &> /dev/null
then
    echo "### 'wget' could not be found. Please install 'wget' and start the script again."
    exit 1
fi

download_gettext
build_gettext
package_gettext

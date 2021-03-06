#!/bin/bash

# You can specify customized boost by defining BOOST_DIR.
# Install boost like this:
#   wget http://downloads.sourceforge.net/project/boost/boost/1.54.0/boost_1_54_0.zip?r=&ts=1442891144&use_mirror=jaist
#   unzip -q boost_1_54_0.zip
#   cd boost_1_54_0
#   ./bootstrap.sh --with-libraries=system,filesystem --with-toolset=gcc
#   ./b2 toolset=gcc cxxflags="-std=c++11 -fPIC" -j8 -d0
#   ./b2 install --prefix=`pwd`/output -d0
# And set BOOST_DIR as:
#   export BOOST_DIR=/path/to/boost_1_54_0/output
if [ -n "$BOOST_DIR" ]
then
    echo "Use customized boost: $BOOST_DIR"
    BOOST_OPTIONS="-DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_DIR -DBoost_NO_SYSTEM_PATHS=ON"
else
    BOOST_OPTIONS=""
fi

if [ $# -eq 1 -a "$1" == "true" ]
then
    echo "Clear builder..."
    rm -rf builder
    mkdir -p builder
    cd builder
    cmake .. -DCMAKE_INSTALL_PREFIX=`pwd`/output -DCMAKE_BUILD_TYPE=Debug $BOOST_OPTIONS
    cd ..
fi

cd builder
make -j4
make install
cd ..


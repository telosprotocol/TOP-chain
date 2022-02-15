mkdir -p cbuild
cd cbuild

osname=`uname` # Linux/Darwin
if [ $osname == "Linux" ]; then
    cmake3 ..
elif [ $osname == "Darwin" ]; then
    cmake ..
fi

make -j4

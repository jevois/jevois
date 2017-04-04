#!/bin/bash
usage="USAGE: jevois-create-module.sh <VendorName> <ModuleName> <DirName>"

if [ "X$1" = "X" ]; then echo $usage; exit 1; fi
if [ "X$2" = "X" ]; then echo $usage; exit 2; fi
if [ "X$3" = "X" ]; then echo $usage; exit 3; fi

if [ -d $3 ]; then echo "Directory [$3] already exists -- ABORT"; exit 4; fi

   
read -p "Create module [$2] from vendor [$1] in new directory [$3]  (Y/n)? "
if [ "X$REPLY" != "Xn" ]; then
    echo "*** Cloning from samplemodule github..."
    mkdir -p $3
    if [ ! -d $3 ]; then echo "Directory [$3] could not be created -- ABORT"; exit 5; fi
    cd $3
    git clone https://github.com/jevois/samplemodule.git

    echo "*** Patching up and customizing..."
    
    /bin/mv samplemodule/* .
    /bin/rm -rf samplemodule

    sed -i "s/SampleVendor/$1/g" CMakeLists.txt

    /bin/mv src/Modules/SampleModule src/Modules/$2
    /bin/mv src/Modules/$2/SampleModule.C src/Modules/$2/${2}.C 

    sed -i "s/SampleModule/$2/g" src/Modules/$2/${2}.C
    sed -i "s/SampleModule/$2/g" src/Modules/$2/postinstall
    sed -i "s/samplemodule/$3/g" CMakeLists.txt
    sed -i "s/SampleModule/$2/g" CMakeLists.txt

    chmod a+x src/Modules/$2/postinstall

    cd ..
    
    echo "*** All done. The following files were created:"
    find $3 -print
fi

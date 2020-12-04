#!/bin/sh


name="ebpc_"
gitversion=$(git rev-parse --short HEAD)
version=$(sed -n "/static string version = /p" ./ca/ca_global.cpp |awk -F '[\"]' '{print $2}')

finalname=${name}""${version}

if [ ${#gitversion} -eq 0 ]
then
    echo "there is no git in your shell"
    exit
else
    finalname=${finalname}"_"${gitversion}
fi;

flag=$(sed -n "/g_testflag = /p" ./ca/ca_global.cpp |awk -F '[ ;]' '{print $4}')

if [ $flag -eq 1 ] 
then
    finalversion=${finalname}"_""testnet"
    echo  "${finalversion}"
else  
    finalversion=${finalname}"_""primarynet"
     echo  "${finalversion}"
fi;

if [ -f ebpc ]
then
    mv ebpc ${finalversion}
else
    echo "ebpc not exist"
fi;
 

#sed -i "s/build_commit_hash.*;/build_commit_hash = \"${gitversion}\";/g" ./ca/ca_global.cpp
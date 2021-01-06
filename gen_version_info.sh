#!/bin/sh
###
 # @Author: your name
 # @Date: 2020-12-24 10:14:42
 # @LastEditTime: 2021-01-06 16:24:12
 # @LastEditors: Please set LastEditors
 # @Description: In User Settings Edit
 # @FilePath: \uenc_src\gen_version_info.sh
### 


name="ebpc_"
gitversion=$(git rev-parse --short HEAD)
version=$(sed -n "/std::string g_LinuxCompatible = /p" ./ca/ca_global.cpp |awk -F '[\"]' '{print $2}')

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
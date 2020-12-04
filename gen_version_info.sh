#!/bin/sh
###
 # @Author: your name
 # @Date: 2020-11-04 20:24:26
 # @LastEditTime: 2020-11-20 15:13:27
 # @LastEditors: Please set LastEditors
 # @Description: In User Settings Edit
 # @FilePath: \uenc_src\gen_version_info.sh
### 


name="ebpc_"
gitversion=$(git rev-parse --short HEAD)

if [ ${#gitversion} -eq 0 ]
then
    echo "there is no git in your shell"
    exit
else
    finalname=${name}${gitversion}
fi;

version=$(sed -n "/versionNum = /p" ./ca/ca_global.cpp |awk -F '[\"]' '{print $2}')

finalname=${finalname}"_"${version}

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
 


###
 # @Author: your name
 # @Date: 2020-11-04 20:24:26
 # @LastEditTime: 2020-11-05 09:00:58
 # @LastEditors: Please set LastEditors
 # @Description: In User Settings Edit
 # @FilePath: \undefinedc:\Users\biz\Desktop\code\uenc_src\tools\gen_proto.sh
### 
#!/bin/bash

SHDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROTODIR=${SHDIR}/../proto/src
PROTOC=${SHDIR}/protoc/protoc
OUTDIR=${SHDIR}/../proto/

for proto in `ls $PROTODIR`
do
    echo "processing--->"${proto}
    $PROTOC -I=$PROTODIR --cpp_out=$OUTDIR ${proto} 
done




# -------------
# for proto in `find $PROTODIR -name *.proto`


# jsonrcp接口文档
---------------------------

说明：jsonrpc标准为2.0，文档中的数据皆为测试数据，请求方式为 POST



## 一、获取高度（get_height）

### 请求
```
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_height"
}
```

### 返回值

```
成功返回：
{
  "jsonrpc": "2.0",
  "id": "1",
  "result": {
    "height": "100"
  }
}
失败返回：
{
    "error": {
        "code": -32601,
        "message": "Method not found"
    },
    "id": "",
    "jsonrpc": "2.0"
}
```

### 字段说明
```
请求：
jsonrpc  字符串类型 2.0标准 (相同字段以下不再重复)
id		 字符串类型 编号，客户端发送什么，服务端回复相同的编号 (相同字段以下不再重复)
method	 字符串类型 调用的方法名 (相同字段以下不再重复)
响应：
result   json对象  调用成功返回的结果信息 (相同字段以下不再重复)
height 	 字符串类型 区块高度
error    json对象  调用出错返回的结果信息 (相同字段以下不再重复)
code     整型  	 错误码 (相同字段以下不再重复)
message  字符串类型 错误描述 (相同字段以下不再重复)
```



## 二、通过高度获取所有交易hash  (get_txids_by_height)

### 请求
```
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_txids_by_height",
  "params": {
    "height": "1"
  }
}
```

### 返回值
```
成功返回：
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": [
        "772298b54a30e8b9e51b677a497369e19c3bc8ad500bf418b968638fd5d2960f",
        "6916d3a37128df383326249abfd57fec11fe693ca1b802bb0e0a9293a688c520",
        "7744719b0014bf6f733b9a0624a78273e0cf90818dd5fb02b623a0229990cebb"
    ]
}
失败返回：
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

### 字段说明
```
请求：
params   json对象		方法所需要的参数(相同字段以下不再重复)
height 	 字符串类型    区块高度
响应：
result   json数组     当前区块高度所有交易hash组成的json数组
```



## 三、根据地址获取余额（get_balance）

### 请求
``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_balance",
  "params": {
    "address": "1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES"
  }
}
```

### 返回值
```
成功返回：
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "balance": "888.666668"
    }
}
失败返回：
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}

```

### 字段说明
```
请求：
address  字符串类型	  钱包地址
响应：
balance  字符串类型	  钱包余额
```



## 四、根据交易hash获取交易详情  (get_tx_by_txid)

### 请求
``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_tx_by_txid",
  "params": {
    "hash": "772298b54a30e8b9e51b677a497369e19c3bc8ad500bf418b968638fd5d2960f"
  }
}
```


### 返回值
```
成功返回：
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "hash": "772298b54a30e8b9e51b677a497369e19c3bc8ad500bf418b968638fd5d2960f",
        "time": 1602641586086592,
        "vin": [
            "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu"
        ],
        "vout": [
            {
                "address": "1BmY6N9EQCbS8haRuJ1NuzHSQkTU5c4F5j",
                "value": "1000.000000"
            },
            {
                "address": "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu",
                "value": "99998998.400000"
            }
        ]
    }
}

失败返回：
没有查找到该笔交易返回：
{
    "error": {
        "code": -32000,
        "message": "not find"
    },
    "id": "1",
    "jsonrpc": "2.0"
}

参数错误返回：
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

### 字段说明
```
请求：
hash  	字符串类型		 	交易hash
响应：
hash  	字符串类型		 	交易hash
time  	无符号64位整型   		时间戳
vin   	json数组		  	  交易转出地址
vout  	json数组		      交易转入地址和金额组成的json对象
address 字符串类型		    交易转入地址
value 	字符串类型		    交易金额 
```



## 五、创建交易体  (create_tx_message)

### 请求
```
{
    "jsonrpc": "2.0",
    "id": "1",
    "method": "create_tx_message",
    "params": {
        "from_addr": ["1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES"],
        "to_addr": [{"addr": "1FoQKZdUNeBXV2nTba6e354m5JrQ4rHYgA", "value": "22.222222"}],
        "fee": "0.555555"
    }
}
```

### 返回值
```
成功返回：
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "tx_data": "EM2nvbfKuOwCIiIxQnVGcERtSDJiSmhxeFFveXY4Y0M4WUwzbVUyVG5VREVTMig4ZjU1M2U5ODA4MzM4MjZhMDIxYWQ5MTU4MDA5N2E5OGVkY2EzM2M3QkQKQgpAMDEwZDJmYTBkNzkwNDEzNDlmM2QwZWFmY2FjMzg5ZTQ4NTM1MzgyYzE1M2VmYzNiYWZlZjFjMTcyNjU5ZjU2YUopCI6rzAoSIjFGb1FLWmRVTmVCWFYyblRiYTZlMzU0bTVKclE0ckhZZ0FKKgjY4M+cAxIiMUJ1RnBEbUgyYkpocXhRb3l2OGNDOFlMM21VMlRuVURFU1JDeyJHYXNGZWUiOjU1NTU1NSwiTmVlZFZlcmlmeVByZUhhc2hDb3VudCI6MywiVHJhbnNhY3Rpb25UeXBlIjoidHgifQ==",
        "tx_encode_hash": "3c9a103d8542750dd048eecf2151b052ed26051f201246089bfc01e508ed7000"
    }
}
失败返回：
创建交易失败返回：
{
    "error": {
        "code": -32000,
        "message": "create fail,error number:-2"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
参数错误返回：
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

### 字段说明
```
请求：
from_addr       交易转出地址组成
to_addr			addr:交易转入地址 value:交易金额
fee             交易燃料费
响应：
tx_data         交易体(base64编码)
tx_encode_hash  交易体hash(待签名信息)
```



## 六、发送交易  (send_tx)

### 请求
```
{
	"jsonrpc": "2.0",
	"id": "1",
	"method": "send_tx",
	"params": {
		"tx_data": "ELvdqOvRuOwCIiIxQnVGcERtSDJiSmhxeFFveXY4Y0M4WUwzbVUyVG5VREVTMig4ZjU1M2U5ODA4MzM4MjZhMDIxYWQ5MTU4MDA5N2E5OGVkY2EzM2M3QkQKQgpAMjRkMjUxMzMxZGFkYjEyMGMyYmYxMDlhZDI2ODllOWNkMDcwYTAyZWJkZWQxNDA1ZTM5MGFlMmVhMDI0YjEzMEopCI6rzAoSIjFGb1FLWmRVTmVCWFYyblRiYTZlMzU0bTVKclE0ckhZZ0FKKgiwua+GAxIiMUJ1RnBEbUgyYkpocXhRb3l2OGNDOFlMM21VMlRuVURFU1JDeyJHYXNGZWUiOjU1NTU1NSwiTmVlZFZlcmlmeVByZUhhc2hDb3VudCI6MywiVHJhbnNhY3Rpb25UeXBlIjoidHgifQ==",
		"tx_signature": "N1ii0dikr0NJRvi7GXkjXOayD+mVcMfXF+49iOmOneYqYj2HHYzNm3Txj/otW/K7Dh3uBJ2Gb4nlTJW2AY3Dog==",
		"public_key": "ICBszM0aHCpWmDdEC3GMBL6DFN7XdWzijF33uvmWKMa1WbvWBk33+G9E4pSztJWlwDkvEt4dW4oGY8/sY2FJBtPG",
		"tx_encode_hash": "b3b8f15852efddbdfe8aa759a2f026488350b6f56a4cae7494ea3cbba0f8a5c5"
	}
}
```
### 返回值
```
成功返回：
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "tx_hash": "e241d6af1b8f8ff58508f14177005b4263d26e32a2e0d0f6b8e98d966cbaa864"
    }
}
失败返回：
验证签名失败返回：
{
    "error": {
        "code": -32000,
        "message": "create fail,error number:-8"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
参数错误返回：
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

### 字段说明
```
请求：
tx_data         字符串类型		交易体(base64编码),创建交易体方法调用后返回的tx_data
tx_signature    字符串类型		对交易体hash(tx_encode_hash)进行签名得到的签名信息,调用动态库GenSign_()方法进行签名
public_key      字符串类型		公钥(base64编码)
tx_encode_hash	字符串类型		交易体hash(待签名信息),创建交易体方法调用后返回的tx_encode_hash
响应：
tx_hash         字符串类型		交易hash(可通过此hash查询完整交易信息)
```



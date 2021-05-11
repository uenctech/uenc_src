## jsonrpc�ӿ��ĵ� :
---------------------------

˵����jsonrpc��׼Ϊ2.0���ĵ��е����ݽ�Ϊ�������ݣ�����ʽΪ POST
Ĭ�϶˿�Ϊ11190�������������ļ�config.json��"http_port"������ָ���˿�ֵ
��ʹ��HTTP����Postman����Curl���з��ʲ���

Postmanʾ��  
![](jsonrpc�ӿ��ĵ�.assets/postman.png)
```
Postman���½�"Request", ѡ��"POST",����URL��ַ��������˿ں�11190���磺192.168.1.51:11190/����
ѡ��Body,ѡ��Rawѡ�����json�������ݣ���{ "jsonrpc": "2.0", "method": "get_height", "id": "1" },
��д��Ϻ󣬵��"Send"��ť���������Ӧ���󲢷�����Ӧ���ݡ�
```

Curlʾ��
```
Curl: curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{ "jsonrpc": "2.0", "method": "get_height", "id": "1" }' 192.168.1.51:11190
�����������ݣ����ɷ���get_height�ӿ�
```


#### һ����ȡ�߶ȣ�get_height��

 ����  
```
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_height"
}
```


�ɹ����أ�  
```
{
  "jsonrpc": "2.0",
  "id": "1",
  "result": {
    "height": "100"
  }
}  
```  
ʧ�ܷ��أ�  
```
{
    "error": {
        "code": -32601,
        "message": "Method not found"
    },
    "id": "",
    "jsonrpc": "2.0"
}
```

 �ֶ�˵��

����  
jsonrpc  �ַ������� 2.0��׼ (��ͬ�ֶ����²����ظ�)
id		 �ַ������� ��ţ��ͻ��˷���ʲô������˻ظ���ͬ�ı�� (��ͬ�ֶ����²����ظ�)
method	 �ַ������� ���õķ����� (��ͬ�ֶ����²����ظ�)  
��Ӧ��  
result   json����  ���óɹ����صĽ����Ϣ (��ͬ�ֶ����²����ظ�)
height 	 �ַ������� ����߶�
error    json����  ���ó����صĽ����Ϣ (��ͬ�ֶ����²����ظ�)
code     ����  	 ������ (��ͬ�ֶ����²����ظ�)
message  �ַ������� �������� (��ͬ�ֶ����²����ظ�)


ʾ��
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "get_height", "id": "1" }' 192.168.1.51:11190
```  
pythonʾ��:


��ȡ�߶�:  
```
def get_height():
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_height"
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print(res.text)
```

#### ����ͨ���߶Ȼ�ȡ���н���hash  (get_txids_by_height)

����:  
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

�ɹ����أ�   
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": [
        "772298b54a30e8b9e51b677a497369e19c3bc8ad500bf418b968638fd5d2960f",
        "6916d3a37128df383326249abfd57fec11fe693ca1b802bb0e0a9293a688c520",
        "7744719b0014bf6f733b9a0624a78273e0cf90818dd5fb02b623a0229990cebb"
    ]
}
```
ʧ�ܷ��أ�  
```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```  
����ĸ߶ȸ�ʽ�������  
```
{
    "error": {
        "code": -1,
        "message": "height is invalid"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```  
����ĸ߶ȳ�����߿�ĸ߶ȣ�  
```
{
    "error": {
        "code": -4,
        "message": "height more than block top "
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

�ֶ�˵��:

����  
    params   json����		��������Ҫ�Ĳ���(��ͬ�ֶ����²����ظ�)  
    height 	 �ַ�������    ����߶�  
��Ӧ��  
    result   json����     ��ǰ����߶����н���hash��ɵ�json����


ʾ��:  
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "get_txids_by_height", "params": {"height": "1"} }' 192.168.1.51:11190
```

pythonʾ��:

ͨ���߶Ȼ�ȡ���н���hash  
 �ӿڣ�def get_txids_by_height()
  
```  
    height = input("������Ҫ��ѯ�ĸ߶�:")
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_txids_by_height",
        "params": {
            "height": height
        }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print(res.text)
```    



#### �������ݵ�ַ��ȡ��get_balance��

 ����

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


�ɹ����أ�

```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "balance": "888.666668"
    }
}
```

ʧ�ܷ��أ�

```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```  

��ѯ��ַ��ʽ�������

```
{
    "error": {
        "code": -1,
        "message": "address is invalid "
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

�ֶ�˵��

����
address  �ַ�������	  Ǯ����ַ

��Ӧ��
balance  �ַ�������	  Ǯ�����


ʾ��:  

```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "id": "1", "method": "get_balance", "params": { "address":"1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES" } }' 192.168.1.51:11190
```  


pythonʾ��


���ݵ�ַ��ȡ��get_balance��  
�ӿڣ�def get_balance()
  
   ```  
    address = input("������Ҫ��ѯ�ĵ�ַ:")
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_balance",
        "params": {
        "address": address
        }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print(res.text)  
   ```



#### �ġ����ݽ���hash��ȡ��������  (get_tx_by_txid)

����:  
```
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_tx_by_txid",
  "params": {
    "hash": "3bb0c305a59c45a35eb48fef3ac5a9f42104a083288b867572fa07b9a7961baa"
  }
}
```
�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "hash": "3bb0c305a59c45a35eb48fef3ac5a9f42104a083288b867572fa07b9a7961baa",
        "time": 1603854991179495,
        "type": "tx"
        "height": ""
        "vin": [
            {
                "address": "1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES",
                "output_index": 0,
                "output_value": "1000.000000",
                "prev_hash": "4df2ac157683a5553503731aa74495c556f46faf11c595b95ee5980f8b5013b0"
            }
        ],
        "vout": [
            {
                "address": "1FoQKZdUNeBXV2nTba6e354m5JrQ4rHYgA",
                "value": "10.000000"
            },
            {
                "address": "1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES",
                "value": "989.000000"
            }
        ]
    }
}
```
û�в��ҵ��ñʽ��׷��أ�  
```
{
    "error": {
        "code": -32000,
        "message": "not find"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```  
�������󷵻أ�  
```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```  

��ϣ���Ȳ�����64��  
```
{
    "error": {
        "code": -1,
        "message": "hash is invalid"
    },
    "id": "1",
    "jsonrpc": "2.0"
}

```
�ֶ�˵��

����  

hash  			�ַ�������		 	����hash
��Ӧ��
hash  			�ַ�������		 	����hash
time  			�޷���64λ����   		ʱ���

type			�ַ�������			���׵�����,����������,ֻ��Ҫ���������������ͣ�    "tx"		  	��������
													  	  					"pledge"  		��Ѻ����
													  	  					"redeem" 		����Ѻ����
height			�ַ�������			��ǰ������������߶�
vin   			json����		  	  ��������
address 		�ַ�������			����ת����ַ
prev_hash		�ַ�������			utxo���ڵĽ���hash
output_index	����				  ����
output_value	�ַ�������			utxo���

vout  			json����		      ����ת���ַ�ͽ����ɵ�json����
address 		�ַ�������		    ����ת���ַ
value 			�ַ�������		    ���׽�� 

ʵ�ʻ��ѵ�fee���㣺vin���output_value ��ȥ vout �������value


ʾ��:  
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "id": "1", "method": "get_tx_by_txid", "params": { "hash": "3bb0c305a59c45a35eb48fef3ac5a9f42104a083288b867572fa07b9a7961baa" } }' 192.168.1.51:11190
```

pythonʾ��

���ݽ���hash��ȡ��������  
  

�ӿڣ�def get_tx_by_txid() 
   ```  
    hash = input("������Ҫ��ѯ�Ľ���hash:")
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_tx_by_txid",
        "params": {
            "hash": hash
        }
    }  
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print(res.text)  
   ```


 
####�塢����������  (create_tx_message)

����   
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

�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "tx_data": "EM2nvbfKuOwCIiIxQnVGcERtSDJiSmhxeFFveXY4Y0M4WUwzbVUyVG5VREVTMig4ZjU1M2U5ODA4MzM4MjZhMDIxYWQ5MTU4MDA5N2E5OGVkY2EzM2M3QkQKQgpAMDEwZDJmYTBkNzkwNDEzNDlmM2QwZWFmY2FjMzg5ZTQ4NTM1MzgyYzE1M2VmYzNiYWZlZjFjMTcyNjU5ZjU2YUopCI6rzAoSIjFGb1FLWmRVTmVCWFYyblRiYTZlMzU0bTVKclE0ckhZZ0FKKgjY4M+cAxIiMUJ1RnBEbUgyYkpocXhRb3l2OGNDOFlMM21VMlRuVURFU1JDeyJHYXNGZWUiOjU1NTU1NSwiTmVlZFZlcmlmeVByZUhhc2hDb3VudCI6MywiVHJhbnNhY3Rpb25UeXBlIjoidHgifQ==",
        "tx_encode_hash": "3c9a103d8542750dd048eecf2151b052ed26051f201246089bfc01e508ed7000"
    }
}  
```


��������ʧ�ܷ��أ�  
```
{
    "error": {
        "code": -32000,
        "message": "create fail,error number:-2"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```  

�������󷵻أ�  
```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```  

�����ʱ��value��ʽ�������("value":1.2345678 ����"value"��"abc"��)  

```
{
    "error": {
        "code": -32602,
        "message": "The value is wrong or More than 6 decimal places"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```


�ֶ�˵��

����  
from_addr       ����ת����ַ���  
to_addr			addr:����ת���ַ value:���׽��  
fee             ����ȼ�Ϸ�  
��Ӧ��  
tx_data         ������(base64����)  
tx_encode_hash  ������hash(��ǩ����Ϣ)


ʾ��
  
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "id": "1", "method": "create_tx_message", "params": { "from_addr": ["1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES"], "to_addr": [{"addr": "1FoQKZdUNeBXV2nTba6e354m5JrQ4rHYgA", "value": "22.222222"}], "fee": "0.555555"} }' 192.168.1.51:11190
```

pythonʾ��


ͨ������create_tx_message rpc�ӿڣ���������������  
�ӿڣ�def create_tx_message()  
 
  ```  
      data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "create_tx_message",
        "params": {
            "from_addr": ["1FJpJQkhunjirwjKm85f1P6LcCGnF4Tfet"],
            "to_addr": [{"addr": "1McVeDa3cM6A9939wKqLmnuxp863fZXXiC", "value": "4.1"}],
            "fee": "0.1"
        }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    result = json.loads(res.text)
    tx_data = result["result"]["tx_data"]
    tx_encode_hash = result["result"]["tx_encode_hash"]   
    dict_list = {'tx_data': tx_data,
                 'tx_encode_hash': tx_encode_hash}
    return_value = json.dumps(dict_list)
    return return_value  
 ```



#### �������ͽ���  (send_tx)

����  

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


�ɹ�����ֵ:
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "tx_hash": "e241d6af1b8f8ff58508f14177005b4263d26e32a2e0d0f6b8e98d966cbaa864"
    }
}  
```

��֤ǩ��ʧ�ܷ��أ�
```
{
    "error": {
        "code": -32000,
        "message": "create fail,error number:-8"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```  

�������󷵻أ�  
```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

�ֶ�˵��

����  
tx_data         �ַ�������		������(base64����),���������巽�����ú󷵻ص�tx_data  
tx_signature    �ַ�������		�Խ�����hash(tx_encode_hash)����ǩ���õ���ǩ����Ϣ,���ö�̬��GenSign_()��������ǩ��  
public_key      �ַ�������		��Կ(base64����)   
tx_encode_hash	�ַ�������		������hash(��ǩ����Ϣ),���������巽�����ú󷵻ص�tx_encode_hash  
��Ӧ��
tx_hash         �ַ�������		����hash(��ͨ����hash��ѯ����������Ϣ)


ʾ��
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "id": "1", "method": "send_tx", "params": { "tx_data":"ELvdqOvRuOwCIiIxQnVGcERtSDJiSmhxeFFveXY4Y0M4WUwzbVUyVG5VREVTMig4ZjU1M2U5ODA4MzM4MjZhMDIxYWQ5MTU4MDA5N2E5OGVkY2EzM2M3QkQKQgpAMjRkMjUxMzMxZGFkYjEyMGMyYmYxMDlhZDI2ODllOWNkMDcwYTAyZWJkZWQxNDA1ZTM5MGFlMmVhMDI0YjEzMEopCI6rzAoSIjFGb1FLWmRVTmVCWFYyblRiYTZlMzU0bTVKclE0ckhZZ0FKKgiwua+GAxIiMUJ1RnBEbUgyYkpocXhRb3l2OGNDOFlMM21VMlRuVURFU1JDeyJHYXNGZWUiOjU1NTU1NSwiTmVlZFZlcmlmeVByZUhhc2hDb3VudCI6MywiVHJhbnNhY3Rpb25UeXBlIjoidHgifQ==", "tx_signature": "N1ii0dikr0NJRvi7GXkjXOayD+mVcMfXF+49iOmOneYqYj2HHYzNm3Txj/otW/K7Dh3uBJ2Gb4nlTJW2AY3Dog==", "public_key": "ICBszM0aHCpWmDdEC3GMBL6DFN7XdWzijF33uvmWKMa1WbvWBk33+G9E4pSztJWlwDkvEt4dW4oGY8/sY2FJBtPG", "tx_encode_hash": "b3b8f15852efddbdfe8aa759a2f026488350b6f56a4cae7494ea3cbba0f8a5c5"} }' 192.168.1.51:11190
```

pythonʾ��


 ����send_tx rpc�ӿڷ��ͽ���  
 @param tx_data ����������(create_tx_message)���ص�tx_data  
 @param tx_signature ����ǩ����Ϣ��generate_sign�����ص�message��Ϣ  
 @param public_key ����Ǯ����ַ����Կ��˽Կ��generate_wallet�����ص�public_key  
 @param tx_encode_hash ����������(create_tx_message)���ص�tx_encode_hash  
        
 �ӿڣ� def send_tx(tx_data, tx_signature, public_key, tx_encode_hash)
 ```   
   data = {
	    "jsonrpc": "2.0",
	    "id": "1",
	    "method": "send_tx",
	    "params": {
		    "tx_data": tx_data,
		    "tx_signature": tx_signature,
		    "public_key": public_key,
		    "tx_encode_hash": tx_encode_hash
	    }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
     result = json.loads(res.text)  
 ```
   



#### �ߡ���ȡ���100��߶ȵ�ƽ������ȼ�Ϸ� ��get_avg_fee��

˵����  
fee�ɿ��������ã�������ͽ���ʱ���õ�feeֵ���ڴ���������õ�feeֵ�����ܻ���ɽ��ײ��ɹ�����ˣ����Բ������100���߶ȵ�����ƽ��feeֵ���趨��ȷ�������ܳɹ���

����

```
{
	"jsonrpc": "2.0",
	"id": "1",
	"method": "get_avg_fee"
}
```


�ɹ����أ�
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "avg_fee": "0.112074"
    }
}  
```  

ʧ�ܷ��أ�  
```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

�ֶ�˵��

��Ӧ��  
avg_fee         �ַ�������		���100���߶�����fee��ƽ��ֵ,����߶Ȳ���100,�������������ƽ��ֵ

ʾ��
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "get_avg_fee", "id": "1" }' 192.168.1.51:11190
```

pythonʾ��


��ȡ���100��߶ȵ�ƽ������ȼ�Ϸ�   
```   
def get_avg_fee():
    data = {
        "jsonrpc": "2.0",
	    "id": "1",
	    "method": "get_avg_fee"
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print(res.text)
```



#### �ˡ�����Ǯ����ַ����Կ��˽Կ��generate_wallet��

����

``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "generate_wallet"
}
```

�ɹ����أ�
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "address": "1BGmh7NgY7spKRBHJkwQTZosaHGiXfynvj",
        "private_key": "xAEF+gTQZ6PmtH3hlmygJpAVxBpKHBa3Zw8iMxRjlbQ=",
        "public_key": "ICD6bienPIel1KE4WmGlQ6bC6M+HiPTw3+et036AUaTVtLr1iV1DMFFx2O9VYi/MUXOZyKK87s/GjPE+eN9A+wEl"
    }
}
```
ʧ�ܷ��أ�  
 
 ```
 {
    "error": {
        "code": -32601,
        "message": "Method not found"
    },
    "id": "",
    "jsonrpc": "2.0"
 }
```

�ֶ�˵��

��Ӧ��  
address  	�ַ�������	  	Ǯ����ַ  
private_key �ַ������� 		base64������˽Կ  
public_key  �ַ������� 		base64�����Ĺ�Կ  


ʾ��:
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "generate_wallet", "id": "1" }' 192.168.1.51:11190
```

pythonʾ��:  
ͨ������generate_wallet rpc�ӿڣ�����Ǯ����ַ����Ӧ�Ĺ�˽Կ   
``` 
def generate_wallet():  
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "generate_wallet"
    }  
    headers = {
        "Content-Type": "application/json"
    }  
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    result = json.loads(res.text)
    address = result["result"]["address"]
    private_key = result["result"]["private_key"]
    public_key = result["result"]["public_key"]
    dict_list = {'address': address,
                 'private_key': private_key, 'public_key': public_key}
    return_value = json.dumps(dict_list)
    return return_value
```



#### �š�����ǩ����Ϣ��generate_sign��

����

``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "generate_sign",
  	"params": {
		"data": "b3b8f15852efddbdfe8aa759a2f026488350b6f56a4cae7494ea3cbba0f8a5c5",
		"private_key": "xAEF+gTQZ6PmtH3hlmygJpAVxBpKHBa3Zw8iMxRjlbQ="
	}
}
```
�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "message": "Ggy2ouJDIZw9/ShvZUwXyVgsAXSFLsxvRCh42elAf+Klit6DJH/jUY6Z3Km/W7VhPKinrsHcaEcwYqIUIwopWQ=="
    }
}  
```  

ʧ�ܷ��أ�  
  
 ```
 {
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
  }
```

�ֶ�˵��


����  
data			�ַ�������		��ǩ����Ϣ, create_tx_message�������ú󷵻ص�tx_encode_hash  
private_key		�ַ�������		base64������˽Կ  
��Ӧ��  
message  		�ַ�������		base64��������ǩ����Ϣ


ʾ��
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "generate_sign", "id": "1", "params": { "data": "b3b8f15852efddbdfe8aa759a2f026488350b6f56a4cae7494ea3cbba0f8a5c5", "private_key": "xAEF+gTQZ6PmtH3hlmygJpAVxBpKHBa3Zw8iMxRjlbQ=" } }' 192.168.1.51:11190
```

pythonʾ��

 ����generate_sign rpc�ӿڽ���ǩ��  
 @param tx_encode_hash ����������(create_tx_message)���ص�tx_encode_hash  
 @param private_key ����Ǯ����ַ����Կ��˽Կ(generate_wallet)���ص�private_key  
```  
def generate_sign(tx_encode_hash, private_key):
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "generate_sign",
  	    "params": {
		    "data": tx_encode_hash,
		    "private_key": private_key
	    }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    result = json.loads(res.text)
    //��ȡǩ��֮�󷵻ص�message*
    message = result["result"]["message"]
    //��message��װ��json��ʽ
    dict_list = {"message": message}
    return_value = json.dumps(dict_list)
    return return_value
```



#### ʮ����ѯ���ڹ���Ľ��ף�get_pending_transaction��

����

``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_pending_transaction",
  	"params": {
		"address": "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"
	}
}
```


�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "total": 1,
        "transaction": [
            {
                "amount": "501",
				"broadstamp": 1620378873518030,
                "from": [
                    "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"
                ],
                "gap": "0.050000",
                "hash": "4303e57195616797f77d7db888ef15d677740d8f10a9a8e29370d35c3cc853fb",
                "timestamp": 1620378873279899,
                "to": [
                    "1HjrxHbBuuyNQDwKMh4JtqfuGiDCLodEwC"
                ],
                "toAmount": [
                    "501.000000"
                ],
                "vin": [
                    "7d9a0cb698db789b5f294343209b94ca69119f02593cb5607069623810f6ed69",
                    "92c45d62b86d37c04f5f873eedfdcb1719eeca9a43e16b206e98101d20baeb0c",
                    "d2c9da85e7b67188c507f40a95cba88c491afca56b863cce6af512638c7b1b1c"
                ]
            }
        ]
    }
}
```
ʧ�ܷ���������ĵ�ַ��ʽ�������  
```
{
    "error": {
        "code": -1,
        "message": "address is invalid"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

�ֶ�˵��


����  
address         �ַ�������      ���׷��𷽵�ַ  
��Ӧ��  
total           ��ֵ����        ���ڹ���Ľ��׵ĸ���  
transaction     ��������        ��������,�������׵Ĺ�ϣ�����𷽣����շ�����ʱ���


ʾ��
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "get_pending_transaction", "id": "1", "params": { "address": "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"} }' 192.168.1.51:11190
```

pythonʾ��


��ѯ���ڹ���Ľ���
�ӿڣ�def get_pending_transaction()  
```
    address = input("������Ҫ��ѯ�ĵ�ַ:")
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_pending_transaction",
  	    "params": {
		    "address": address
	    }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print(res.text)
```



#### ʮһ����ѯʧ�ܵĽ��ף�get_failure_transaction��

����

``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_failure_transaction",
  	"params": {
		"address": "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"
	}
}
```


�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "total": 1,
        "transaction": [
            {
                "amount": "500",
                "from": [
                    "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"
                ],
                "gap": "0.050000",
                "hash": "13f9730d0ce5fe401352f42fdce3677e324d15518857c02e0aafc6b5456a7676",
                "timestamp": 1609313648455902,
                "to": [
                    "1HjrxHbBuuyNQDwKMh4JtqfuGiDCLodEwC"
                ],
                "toAmount": [
                    "500.000000"
                ],
                "vin": [
                    "b8930d79b8ecbdd2141d3b4fa85fa7dc0e4c6b3c3e30a379d573aacd34299b18",
                    "c99ac37f9a9c591e51ea31551455f3662eac4e54a1c27923a81e7966c0eadbfa",
                    "08cb13dea1510860de5549a71c8142e16af6698b9b0d9bea3a813789727d084f"
                ]
            }
        ]
    }
}
```
����ʧ��


����ĵ�ַ��ʽ�������  
```
{
    "error": {
        "code": -1,
        "message": "address isinvalid"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```

�ֶ�˵��

����  
address         �ַ�������      ���׷��𷽵�ַ  
��Ӧ��  
total           ��ֵ����        ʧ�ܵĽ��׵ĸ���   
transaction     ��������        ʧ�ܵĽ�������,�������׵Ĺ�ϣ�����𷽣����շ�����ʱ���


ʾ��
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "get_failure_transaction", "id": "1", "params": { "address": "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"} }' 192.168.1.51:11190
```

pythonʾ��


��ѯʧ�ܵĽ���  
```
def get_failure_transaction():
    address = input("������Ҫ��ѯ�ĵ�ַ:")
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_failure_transaction",
  	    "params": {
		    "address": address
	    }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    result = json.loads(res.text)
```



#### ʮ������ȡ����Ϣ�б�get_block_info_list��

����

``` 
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_block_info_list",
  	"params": {
		"index": "15",
        "count": "3",
        "type": "0"
	}
}
```


�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "height": [
            [
                {
                    "block_hash": "b4f74ea3a735a0e6de5c4041bbecfc1b6e2a30156ad41ade7e98c9992e3141ec",
                    "block_height": 15,
                    "block_time": 1611132153984580,
                    "tx": {
                        "amount": "3099.000000",
                        "from": [
                            "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu"
                        ],
                        "hash": "b9b417999b0e4d165d822e9fa9c8fdc553fdd9d6affb054d0a258f6d3db352ee",
                        "to": [
                            "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"
                        ]
                    }
                }
            ],
            [
                {
                    "block_hash": "9870c60872e171b747f35e2f4e876e0792833cb8a258151e38aa1c7f72a52734",
                    "block_height": 14,
                    "block_time": 1611131849651811,
                    "tx": {
                        "amount": "3983.000000",
                        "from": [
                            "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu"
                        ],
                        "hash": "98a483aa1c0b77fed0b69c36888fe6e953fe96d5432a17704e76e4f5a5bc2d64",
                        "to": [
                            "1TT8sdzyPhqSmSx7Wdmn1ECeEHZKosh6v"
                        ]
                    }
                }
            ],
            [
                {
                    "block_hash": "e79fb3d28be54e12a7e5ae9c21d91cefc6ba0d8f25681717d07348b569083f3f",
                    "block_height": 13,
                    "block_time": 1611050624949290,
                    "tx": {
                        "amount": "2349.000000",
                        "from": [
                            "1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu"
                        ],
                        "hash": "bfe2bf795003c2960d09ad03b1d9dd5bc2109c3eba3fe790fd2001f43ce1fc8b",
                        "to": [
                            "1MpeeKXwH1ArnMJ85D161yfH1us471J86X"
                        ]
                    }
                }
            ]
        ]
    }
}  
```  
ʧ�ܷ���  
����������index��ʽ����  
```
{
    "error": {
        "code": -32602,
        "message": "index Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```  
����������count��ʽ����  
```
{
    "error": {
        "code": -32602,
        "message": "count Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```

����������count����Ϊ0��  
```
{
    "error": {
        "code": -1,
        "message": "count is not equal zero��"
    },
    "id": "1",
    "jsonrpc": "2.0"
}  
```  
����������type��ʽ����  
```
 {
    "error": {
        "code": -32602,
        "message": "type Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
 }
```

�ֶ�˵��

����  
index           ��ֵ����        �������ʼ������ַ������ָ��0���ӵ�ǰ��߿��ȡ  
count           ��ֵ����        Ҫ��ȡ�Ŀ�ĸ������Ӹ�����г�  
type            ��ֵ����        Ĭ��Ϊ0���Ժ���չʹ��  
��Ӧ��  
height          ��������        ÿһ�߶Ȳ�Ŀ�����  
block_hash      �ַ�������      ��Ĺ�ϣ   
block_height    ��ֵ����        �����ڵĸ߶�  
block_time      ��ֵ����        �����ʱ��  
amount          �ַ�������      ���׽��  
from            ��������        ���׷�����  
hash            �ַ�������      ���׹�ϣ  
to              ��������        ���׽�����  


ʾ��

```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "get_block_info_list", "id": "1", "params": { "index": "15", "count": "3", "type":"0" } }' 192.168.1.51:11190
```

pythonʾ��

��ȡ����Ϣ�б�  
�ӿڣ�def get_block_info_list()  
```
    data = {
        "jsonrpc": "2.0",
        "id": "1",
        "method": "get_block_info_list",
  	    "params": {
		    "index": "15",
            "count": "3",
            "type": "0"
	    }
    }
    headers = {
        "Content-Type": "application/json"
    }
    res = requests.post(
        url=domain,
        data=json.dumps(data),
        headers=headers)
    print("get_block_info_list res.text:", res.text)
    result = json.loads(res.text)
```

#### ʮ����ȷ�Ͻ����Ƿ�ɹ���confirm_transaction��
����

```
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "confirm_transaction",
  	"params": {
		"tx_hash": "d7ef410796ffa9ef60982c3470f5d816c28a4ea2d3c5299228ef2f5997bf8221"
	}
}
```

�ɹ����أ�

```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "nodeid": [
            "d0b206ec2cee4c04b614d7dc4f9b83584269acd8",
            "c0f710f6b70588bd92983bf50ac710d94f913141",
            "8e41e25eb4ee88a52c34f159d33ada701ce68469",
            "5addc8e04079cc00c49754833b81f67b9458cff5",
            "797a07e6ff413bb35c75243efa087fdc714e0a7d",
            "bc37516cdea7acf3938049219698b9d7db493eba",
            "003f2d85999a6ad668e66dc7cf1ec20dfc1aa9c0"
        ],
        "success": 1,
        "total": 7
    }
}
```

ʧ�ܷ��أ�

```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "nodeid": null,
        "success": 0,
        "total": 0
    }
}
```

tx_hash������ʽ����

```
{
    "error": {
        "code": -32700,
        "message": "Parse error"
    },
    "id": "",
    "jsonrpc": "2.0"
}
```


�ֶ�˵��

����  
```
tx_hash           �ַ�������       ���ν��׵Ľ��׹�ϣ 
```

��Ӧ�� 
``` 
nodeid          ��������        ���سɹ�����Ľڵ�ID
total          	��ֵ����        �ɹ����ν��׽ڵ�ȷ��֮�󷵻صĸ��� 
success      	��������         �ɹ����
```

ʾ��

```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "method": "confirm_transaction", "id": "1", "params": { "tx_hash": "d7ef410796ffa9ef60982c3470f5d816c28a4ea2d3c5299228ef2f5997bf8221"} }' 192.168.1.51:11190
```
#### ʮ�ġ����ݽ��׹�ϣ�ͽڵ�ID��ȡ�������飨get_tx_by_txid_and_nodeid��
����:  
```
{
  "jsonrpc": "2.0",
  "id": "1",
  "method": "get_tx_by_txid",
  "params": {
    "hash": "3bb0c305a59c45a35eb48fef3ac5a9f42104a083288b867572fa07b9a7961baa",
	"nodeid":"d0b206ec2cee4c04b614d7dc4f9b83584269acd8"
  }
}
```
�ɹ����أ�  
```
{
    "id": "1",
    "jsonrpc": "2.0",
    "result": {
        "hash": "3bb0c305a59c45a35eb48fef3ac5a9f42104a083288b867572fa07b9a7961baa",
        "time": 1603854991179495,
        "type": "tx"
        "height": ""
        "vin": [
            {
                "address": "1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES",
                "output_index": 0,
                "output_value": "1000.000000",
                "prev_hash": "4df2ac157683a5553503731aa74495c556f46faf11c595b95ee5980f8b5013b0"
            }
        ],
        "vout": [
            {
                "address": "1FoQKZdUNeBXV2nTba6e354m5JrQ4rHYgA",
                "value": "10.000000"
            },
            {
                "address": "1BuFpDmH2bJhqxQoyv8cC8YL3mU2TnUDES",
                "value": "989.000000"
            }
        ]
    }
}
```
û�в��ҵ��ñʽ��׷��أ�  
```
{
    "error": {
        "code": -32000,
        "message": "not find"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```  
�������󷵻أ�  
```
{
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": "1",
    "jsonrpc": "2.0"
}
```  

��ϣ���Ȳ�����64��  
```
{
    "error": {
        "code": -1,
        "message": "hash is invalid"
    },
    "id": "1",
    "jsonrpc": "2.0"
}

```
�ֶ�˵��

����  

hash  			�ַ�������		 	����hash
��Ӧ��
hash  			�ַ�������		 	����hash
time  			�޷���64λ����   		ʱ���

type			�ַ�������			���׵�����,����������,ֻ��Ҫ���������������ͣ�    "tx"		  	��������
													  	  					"pledge"  		��Ѻ����
													  	  					"redeem" 		����Ѻ����
height			�ַ�������			��ǰ������������߶�
vin   			json����		  	  ��������
address 		�ַ�������			����ת����ַ
prev_hash		�ַ�������			utxo���ڵĽ���hash
output_index	����				  ����
output_value	�ַ�������			utxo���

vout  			json����		      ����ת���ַ�ͽ����ɵ�json����
address 		�ַ�������		    ����ת���ַ
value 			�ַ�������		    ���׽�� 

ʵ�ʻ��ѵ�fee���㣺vin���output_value ��ȥ vout �������value


ʾ��:  
```
curl -i -X POST -H "Content-Type: application/json; indent=4" -d '{"jsonrpc": "2.0", "id": "1", "method": "get_tx_by_txid_and_nodeid", "params": { "hash": "3bb0c305a59c45a35eb48fef3ac5a9f42104a083288b867572fa07b9a7961baa","nodeid":"d0b206ec2cee4c04b614d7dc4f9b83584269acd8" } }' 192.168.1.51:11190
```
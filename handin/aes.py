# https://github.com/vito929/Network-Security-exercise/blob/main/01_symmetric_encryption/ppmcrypt_solution.py
from Crypto.Util.Padding import pad, unpad
from Crypto.Cipher import AES
import secrets
from Crypto.Util.Padding import pad, unpad
import sys

data = b"User:TeloSwift;;User:TeloSwift;;User:TeloSwift;;User:TeloSwift;;"
# print(sys.getsizeof(data))
key = b"2222222222222222"
# key = secrets.token_bytes(16)
# print(sys.getsizeof(key))

def xor(var, key):
    return bytes(a ^ b for a, b in zip(var, key))

# ECB
# pad_data = pad(data,16)
# print(pad_data)
print('ECB:')
cipher = AES.new(key, AES.MODE_ECB)
ciphertext = cipher.encrypt(data)
print('encrypted:',ciphertext.hex())
ba = bytearray(ciphertext)
lastb = ba[-17].to_bytes(1, byteorder='big')
ba[-17] = int.from_bytes(xor(lastb, b'\x01'), byteorder='big')
ciphertext = bytes(ba)
print('flip last:',ciphertext.hex())
pad_plain = cipher.decrypt(ciphertext)
# plaintext = unpad(pad_plain, 16)
print('decrypted:',pad_plain.hex())


print("CBC:")
# CBC
iv = b"3333333333333333"
cipher = AES.new(key,AES.MODE_CBC,iv=iv)
ciphertext = cipher.encrypt(data)
print('encrypted:',ciphertext.hex())

ba = bytearray(ciphertext)
lastb = ba[-20].to_bytes(1, byteorder='big')
ba[-20] = int.from_bytes(xor(lastb, b'\x01'), byteorder='big')
ciphertext = bytes(ba)
print('flip last:',ciphertext.hex())

cipher = AES.new(key,AES.MODE_CBC,iv=iv)
pad_plain = cipher.decrypt(ciphertext)
print('decrypted:',pad_plain.hex())


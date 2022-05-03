# https://github.com/vito929/Network-Security-exercise/blob/main/01_symmetric_encryption/ppmcrypt_solution.py
from Crypto.Util.Padding import pad, unpad
from Crypto.Cipher import AES
import secrets
from Crypto.Util.Padding import pad, unpad
import sys

data = b"1111111111111111"
print(sys.getsizeof(data))
key = b"2222222222222222"
# key = secrets.token_bytes(16)
print(sys.getsizeof(key))

# ECB
# pad_data = pad(data,16)
# print(pad_data)
cipher = AES.new(key, AES.MODE_ECB)
ciphertext = cipher.encrypt(data)
print_cipher = ciphertext.hex()
print(print_cipher)
pad_plain = cipher.decrypt(ciphertext)
# plaintext = unpad(pad_plain, 16)
print(pad_plain)


print("cbc")
# CBC
iv = b"3333333333333333"
aes = AES.new(key,AES.MODE_CBC,iv=iv)
cipher_cbc = aes.encrypt(data)

print_cbc = cipher_cbc.hex()
print(print_cbc)



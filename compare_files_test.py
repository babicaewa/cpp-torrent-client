import hashlib

def calculate_file_hash(file_path, hash_func=hashlib.sha256):
    hash_obj = hash_func()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            hash_obj.update(chunk)
    return hash_obj.hexdigest()


file1 = "/Users/Alex/Downloads/debian-mac-12.7.0-amd64-netinst.iso(fromthesource).iso"
file2 = "/Users/Alex/Desktop/cpp-torrent-client/debian-mac-12.7.0-amd64-netinst.iso"

# Hash comparison as initial check
if calculate_file_hash(file1) == calculate_file_hash(file2):
    print("Files are identical :D")
else:
    print("Files are not identical :(")



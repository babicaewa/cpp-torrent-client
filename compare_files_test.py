import hashlib
import os

# Calculates the hashes of two files, to make sure they match

def calculate_file_hash(file_path, hash_func=hashlib.sha256):
    hash_obj = hash_func()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            hash_obj.update(chunk)
    print(f"Hash: {hash_obj.hexdigest()}")
    return hash_obj.hexdigest()

downloaded_files_folder = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'downloaded_files')

#testing purposes, change directories as needed

file1 = "/Users/Alex/Downloads/debian-mac-12.7.0-amd64-netinst.iso(fromthesource).iso"             #debian os ~660mb
file2 = os.path.join(downloaded_files_folder, "debian-mac-12.7.0-amd64-netinst.iso")

#file1 = "/Users/Alex/Downloads/2024-11-13-raspios-bookworm-armhf-lite(fromthesource).img.xz"    
#file2 = os.path.join(downloaded_files_folder, "2024-11-13-raspios-bookworm-armhf-lite.img.xz")    #raspberry pi lite os ~535mb

#file1 = "/Users/Alex/Downloads/2024-11-13-raspios-bookworm-armhf-full(fromthesource).img.xz"      #full raspberry pi os with desktop & recommended software ~2.86gb
#file2 = os.path.join(downloaded_files_folder, "2024-11-13-raspios-bookworm-armhf-full.img.xz") 


if calculate_file_hash(file1) == calculate_file_hash(file2):
    print("Files are identical :D")
else:
    print("Files are not identical :(")



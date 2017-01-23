# Lzfse-Char-Dev
Kerenel module for character device, used to test in-kernel lzfse library.

## Building
Project `Makefile` expects kernel source tree to be in directory `../linux`.
Also, patch that adds lzfse support should be applied.
To build module, run:
```
$ make
```

## Preparation
Then, module can be inserted as usual
```
# insmod lzfse_cdev.ko
```
Next, create two character devices, for example `lzfse_encode` and `lzfse_decode`.
Major number is 566, minor number for encoder is 0, for decoder is 1:
```
# mknod lzfse_encode c 566 0
# mknod lzfse_decode c 566 1
```

## Usage
To compress data
```
$ cat file > lzfse_encode
$ cat lzfse_encode > file.lzfse
```
To decompress
```
$ cat file.lzfse > lzfse_decode
$ cat lzfse_decode > file
```

_Be careful_ - data buffers have limited size and are shared between all created devices

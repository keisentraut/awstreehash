# awstreehash.c

This is a simple, fast and small command line tool to calculate a Amazon AWS Glacier SHA256 Tree Hash.
See [http://docs.aws.amazon.com/amazonglacier/latest/dev/checksum-calculations.html](http://docs.aws.amazon.com/amazonglacier/latest/dev/checksum-calculations.html) for the documentation of the hash format.
The only dependency is libcrypto from OpenSSL or its variant.

# Compiling
```
$ make
gcc -O3 -pedantic -Wall -o awstreehash awstreehash.c -lcrypto
strip awstreehash
```

# Usage
## reading from stdin
```
$ echo -n test | ./awstreehash -
9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08  -
```

## reading from file(s)
```
$ dd if=/dev/zero of=/tmp/test bs=1024 count=2048
2048+0 records in
2048+0 records out
2097152 bytes (2.1 MB, 2.0 MiB) copied, 0.00272925 s, 768 MB/s
$ ./awstreehash /tmp/test
861890b487038d840e9d71d43bbc0fd4571453fb9d9b1f370caa3582a29b0ec7  test
```


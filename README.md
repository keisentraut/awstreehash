# awstreehash.c

This is a simple, fast and small command line tool to calculate a Amazon AWS Glacier SHA256 Tree Hash.
See [http://docs.aws.amazon.com/amazonglacier/latest/dev/checksum-calculations.html](http://docs.aws.amazon.com/amazonglacier/latest/dev/checksum-calculations.html) for the documentation of the hash format.
The only dependency is libcrypto from OpenSSL or one of its variants like LibreSSL.

Please note that I wrote this in 2017 where the only way to store data in AWS Glacier was by using the Glacier API.
It's now easier to upload data to AWS Glacier because [you can (and probably should) use the S3 API now](https://aws.amazon.com/about-aws/whats-new/2018/11/s3-glacier-api-simplification/).

# Compiling

Just run make.

```
$ make
rm -f awstreehash
gcc -O3 -pedantic -Wall -o awstreehash awstreehash.c -lcrypto
strip awstreehash
```

# Usage

Usage is identical to sha256sum.

Commandline arguments can be one or more filenames or "-" for stdin.
If no commandline arguments are given, stdin is used for input.

## reading from stdin

Let's hash the four byte string "test":

```
$ echo -n test | ./awstreehash
9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08  -
```

We could also add a dash if we want to read from stdin.
This is mainly useful if you want to read from stdin and from some other files simultaneously.

```
$ echo -n test | ./awstreehash -
9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08  -
```

## reading from file(s)

Let's create an example file which is larger than 1 megabyte.

```
$ dd if=/dev/zero of=/tmp/test bs=1024 count=2048
2048+0 records in
2048+0 records out
2097152 bytes (2.1 MB, 2.0 MiB) copied, 0.00272925 s, 768 MB/s
```

We now have a 2 MiB big file which consists of zeros.
Let's calculate its AWS glacier hash:

```
$ ./awstreehash /tmp/test
861890b487038d840e9d71d43bbc0fd4571453fb9d9b1f370caa3582a29b0ec7  test
```

Because the file is larger than 1MiB, this differs from the sha256sum:

```
$ sha256sum /tmp/test
5647f05ec18958947d32874eeb788fa396a05d0bab7c1b71f112ceb7e9b31eee  test
```



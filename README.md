# web-server-cgi-facebooc

This project is a combination of two projects, [web-server-cgi](https://github.com/embedded2016/web-server-cgi) and [facebooc](https://github.com/jserv/facebooc), for [the course](http://wiki.csie.ncku.edu.tw/embedded/schedule) of National Cheng Kung University.

Proof-of-concept Facebook clone in C.
The only dependency is SQLite3.

# Prerequisites

Install SQLite development package:
```bash
sudo apt-get install libsqlite3-dev
```

# Build and Run

```bash
cd web-server-cgi-facebooc
make
cd bin
./cgi_server_test
```

## License

See the [LICENSE](LICENSE.md) file for license rights and limitations (MIT).

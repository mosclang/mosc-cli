## **Introduction**

Mosc is tiny interpreted programming language with the main purpose to be embedded inside a host application. The host 
application is resposible for mosc VM instantiation and management. `mosc-cli` acts as a host application and allows an
on-fly execution of mosc code and mosc file.  
It is backed by libuv to implement IO functionality, and is a work in progress.  

This cli tool is inspired a lot from [Wren Cli](https://github.com/wren-lang/wren-cli)  

## **Build**

You can follow bellow steps to build this project 
* `git clone https://github.com/mosclang/mosc-cli.git mosc-cli`
* `cd mosc-cli`
* `cmake -DCMAKE_BUILD_TYPE=Release .`
* `cmake --build <build-directory> --target moscc -- -j 8`

You will have binary file built under the build directory as `moscc` which you can execute

## **Feature**

The cli includes bellow features (modules)
### `Os` module

### `IO` module

### `repl` module

### `scheduler` module

### `timer` module


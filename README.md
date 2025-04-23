# ChessOverSockets
Server-Client console application for playing chess over Unix sockets.

This project contains both a **client** and a **server** component. The project is built using **CMake**.


## Requirements

- CMake (version 3.10 or higher)
- A C++ compiler (e.g., GCC, Clang, MSVC)

## Building the Project

Follow these steps to build the project:

### 1. Clone the repository

If you haven’t already cloned the repository, you can do so with:

```sh
git https://github.com/K-Triple-A/ChessOverSockets
cd ChessOverSockets
```

### 2. Create a Build Directory
```sh
mkdir build
cd build
```
### 3. Run CMake to Configure the Project
```sh
cmake ..
```
### 4. Build the Project
```sh
make
# make client (client only)
# make server (server only)
```
### 5. Run the Executables
```sh
./server
./client
```
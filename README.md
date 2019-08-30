# INKTTY

Work in progress.

## How to cross-compile for ARM

Unfortunately, the ebook reader I'm targeting here (a Kobo Aura HD) runs an ancient Linux kernel (2.6.35.3). This kernel version isn't supported by modern versions of glibc. The best way to compile applications for older Linux kernels is to install **Ubuntu 14.04** in either a VM or a Docker container. I'd generally recommend using a VM with Ubuntu 14.04 server. You can find downloads for the CD ISOs here: 

### Installing prerequisites

#### Required Packages
Once you're in a **Ubuntu 14.04** shell, install the following packages:
```sh
sudo apt install \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    python-3.5 \
    git \
    pkg-config \
    re2c
```

*Optional:* You can test your ARM executables by installing the `qemu` package. This will register a `binfmt_misc` handler that allows you to transparently run ARM executables on the build machine.

#### Meson

Install `meson` by running
```sh
wget https://github.com/mesonbuild/meson/releases/download/0.51.2/meson-0.51.2.tar.gz
tar -xf meson-0.51.2.tar.gz
```
In the directory `meson-0.51.2` create a file `meson` with the following content:
```sh
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
python3.5 "$DIR/meson.py" $*
```
Mark the file as executable by running
```
chmod +x meson
```

#### Ninja

Install and compile `ninja` by running
```sh
wget https://github.com/ninja-build/ninja/archive/v1.9.0.tar.gz
tar -xf v1.9.0.tar.gz
cd ninja
./configure.py --bootstrap
```

#### Freetype

Install and compile `freetype` by running
```sh
wget https://download.savannah.gnu.org/releases/freetype/freetype-2.10.0.tar.bz2
tar -xf freetype-2.10.0.tar.bz2
cd freetype-2.10.0
mkdir tar
./configure --disable-shared --host=arm-linux-gnueabihf --prefix="$( pwd )/tar"
make -j
make install
```

### Download Inktty and Compile

Now, all that is left is to clone the repository and to compile the executable.

First, we have to add `meson` and `ninja` to the path, as well as set the PKG_CONFIG_PATH to the directory where we installed `freetype`:
```
export PATH=$HOME/meson-0.51.2:$PATH
export PATH=$HOME/ninja-1.9.0:$PATH
export PKG_CONFIG_PATH=$HOME/freetype-2.10.0/tar/lib/pkgconfig
```
Adjust the above paths in case you didn't place the dependencies in your home directory.

```
git clone https://github.com/astoeckel/inktty --recursive
cd inktty; mkdir build_arm; cd build_arm
meson .. --cross-file ../meson_cross_arm.build
ninja
```

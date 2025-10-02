# Zenbleed
![logo](https://github.com/CSCI591USCA/Zenbleed/blob/main/assets/images/logo.png)
---

### Building
Required dependencies (Debian/Ubuntu):
```
build-essential libasound2-dev libjack-jackd2-dev ladspa-sdk libcurl4-openssl-dev libfreetype-dev libfontconfig1-dev \
libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
libwebkit2gtk-4.1-dev libglu1-mesa-dev mesa-common-dev
```

Build using the following commands:
```
git clone https://github.com/CSCI591USCA/Zenbleed.git --recurse-submodules
cd Zenbleed
mkdir build && cd build 
cmake ..
make # For a faster build, use 'make -j <num_cores>'
```

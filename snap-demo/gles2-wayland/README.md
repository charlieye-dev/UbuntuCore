gles2-wayland
=============

a gles2 demo that run on wayland backend.


## Pre
* Install [mir-kioski](https://snapcraft.io/blog/mir-kiosk-uses-mir)
```
$ snap install mir-kiosk
```
* Install [classic](https://kyrofa.com/posts/ros-on-arm64-with-ubuntu-core)
```
$ snap install --edge classic --devmode
```
* Install snapcraft in classic environment
```
$ sudo snap run classic
$ sudo apt-get install snapcraft
```

## Build
* Enter gles2-wayland project dirctory
* Build via snapcraft
```
$ snapcraft
```
* Exit classic environment

## Install
```
$ snap install gles2-wayland_xxx.snap
```

## Run
```
$ snap run gles2-wayland
```

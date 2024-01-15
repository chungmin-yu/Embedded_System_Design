# opencv-face

## opencv-contrib
- Download opencv_contrib - [link](https://github.com/opencv/opencv_contrib/tree/3.4.7)
- 版本為 3.4.7

## Build
大概看一下 opencv-contrib 的 readme

1. `sudo cmake-gui`
2. `OPENCV_EXTRA_MODULES_PATH` 選 opencv_contrib/modules
3. add entry 把要的 flag 加上去打勾, 我只用到 `face` 這個 module
```
BUILD_opencv_legacy=OFF
```

## Modify
1. `BUILD_opencv_cvv` 不要打勾
2. `BUILD_PROTOBUF` & `WITH_PROTOBUF` 要打勾, `PROTOBUF_UPDATE_FILES` 不要打勾
3. `ENABLE_FAST_MATH` 打勾, `OPENCV_ENABLE_NONFREE` 打勾

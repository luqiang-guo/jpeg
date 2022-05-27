# g++  test_opencv.cpp -o  test_opencv \
# -I/home/guoluqiang/jpeg/opencv-out/include \
# -L/home/guoluqiang/jpeg/opencv-out/lib \
# -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
# -Wl,-rpath='/home/guoluqiang/jpeg/opencv-out/lib'


g++  google_test.cpp -o  google_test \
-I/home/guoluqiang/jpeg/opencv-out/include \
-L/home/guoluqiang/jpeg/opencv-out/lib \
-lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
-Wl,-rpath='/home/guoluqiang/jpeg/opencv-out/lib'

# g++  test_parallel_opencv.cpp -o  test_parallel_opencv \
# -I/home/guoluqiang/jpeg/opencv-out/include \
# -L/home/guoluqiang/jpeg/opencv-out/lib \
# -Ijpeg-out/include/ -Ljpeg-out/lib/ \
# -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
# -ljpeg -fopenmp -lpthread -lm \
# -Wl,-rpath='./jpeg-out/lib/' \
# -Wl,-rpath='/home/guoluqiang/jpeg/opencv-out/lib'


# g++ -g test_partial_resize.cpp -o  test_partial_resize \
# -I/home/guoluqiang/jpeg/opencv-out/include \
# -L/home/guoluqiang/jpeg/opencv-out/lib \
# -Ijpeg-out/include/ -Ljpeg-out/lib/ \
# -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
# -ljpeg -fopenmp -lpthread -lm \
# -Wl,-rpath='./jpeg-out/lib/' \
# -Wl,-rpath='/home/guoluqiang/jpeg/opencv-out/lib'




# g++ -g test_nngraph_cpu.cpp -o  test_nngraph_cpu \
# -I/home/guoluqiang/jpeg/opencv-out/include \
# -L/home/guoluqiang/jpeg/opencv-out/lib \
# -Ijpeg-out/include/ -Ljpeg-out/lib/ \
# -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
# -ljpeg -fopenmp -lpthread -lm \
# -Wl,-rpath='./jpeg-out/lib/' \
# -Wl,-rpath='/home/guoluqiang/jpeg/opencv-out/lib'


# g++ -g test_parallel_partial_resize.cpp -o  test_parallel_partial_resize \
# -I/home/guoluqiang/jpeg/opencv-out/include \
# -L/home/guoluqiang/jpeg/opencv-out/lib \
# -Ijpeg-out/include/ -Ljpeg-out/lib/ \
# -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
# -ljpeg -fopenmp -lpthread -lm \
# -Wl,-rpath='./jpeg-out/lib/' \
# -Wl,-rpath='/home/guoluqiang/jpeg/opencv-out/lib'

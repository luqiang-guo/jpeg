# gcc crop.c -o crop -Ijpeg-out/include/ -Ljpeg-out/lib/ -lgcc_s -Wl,-Bstatic -ljpeg -lturbojpeg
gcc crop.c -o crop -Ijpeg-out/include/ -Ljpeg-out/lib/ -ljpeg  -Wl,-rpath='./jpeg-out/lib/'
# gcc test_jpeg_decoder.c -o jpeg_decoder -Ijpeg-out/include/ -Ljpeg-out/lib/ -ljpeg  -Wl,-rpath='./jpeg-out/lib/'
# gcc crop.c -o crop -Ijpeg-out/include/ -Ljpeg-out/lib/ -ljpeg  -Wl,-rpath='./jpeg-out/lib/'
# gcc test_crop.c -o test_crop -Ijpeg-out/include/ -Ljpeg-out/lib/ -ljpeg  -Wl,-rpath='./jpeg-out/lib/'
# gcc parallel_partial.c -o parallel_partial -Ijpeg-out/include/ -Ljpeg-out/lib/ -ljpeg  -fopenmp -lpthread  -Wl,-rpath='./jpeg-out/lib/'
# gcc test_parallel_partial.c -o  test_parallel_partial -Ijpeg-out/include/ -Ljpeg-out/lib/ -ljpeg  -fopenmp -lpthread -lm -Wl,-rpath='./jpeg-out/lib/'


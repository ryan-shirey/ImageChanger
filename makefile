pkg-config:
	g++ bouncer.cpp -o bouncer `pkg-config --cflags --libs libavutil libavformat libavcodec libswscale`

clean:
	rm -r *.spff ; rm -r *.o ; rm -r *.mp4; rm -r bouncer
movie:
	ffmpeg -framerate 30 -i frame%d.spff -vb 40M movie.mp4

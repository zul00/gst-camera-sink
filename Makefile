PROJECT=camera_sink
PKG_CFG = `pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0`

SRC=$(addsuffix .c,$(PROJECT))

.DEFAULT_GOAL: all

# ALL
.PHONY: all
all: $(PROJECT)
$(PROJECT): $(SRC)
	gcc $(SRC) -o $(PROJECT) $(PKG_CFG)


# CLEAN
.PHONY: clean
clean:
	rm $(PROJECT)

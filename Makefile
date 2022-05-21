OUTPUT_DIR = build
OUTPUT = ${OUTPUT_DIR}/conway

FILES = src/conway.cpp src/utils.cpp

build: ${FILES}
	@mkdir -p ${OUTPUT_DIR}
	@g++ src/conway.cpp -lGL -lglut -lGLEW -Ilibs -lSDL2 -lOpenGL -o ${OUTPUT}

run: build
	@${OUTPUT}
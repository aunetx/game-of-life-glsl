OUTPUT_DIR = build
OUTPUT = ${OUTPUT_DIR}/conway

FILES = src/main.cpp src/utils.cpp

build: ${FILES}
	@mkdir -p ${OUTPUT_DIR}
	clang++ src/main.cpp -lGL -lglut -lGLEW -Ilibs -lSDL2 -lOpenGL -o ${OUTPUT}

run: build
	@${OUTPUT}
clang++ src/*.cpp -o RLPlaneSim \
  -std=c++17 \
  -I/opt/homebrew/include \
  -L/opt/homebrew/lib \
  -lraylib \
  -framework CoreVideo \
  -framework IOKit \
  -framework Cocoa \
  -framework GLUT \
  -framework OpenGL

open RLPlaneSim

#works for compiling the game on mac (need to have raylib installed via homebrew)
#run the file `RLPlaneSim`` on mac

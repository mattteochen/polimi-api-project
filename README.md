## Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./a.out

## Callgrind
valgrind --tool=callgrind [callgrind options] your-program [program options]

test: *.cpp *.h Makefile
	clang++ --stdlib=libc++ --std=c++20 -Wall -Wextra -pedantic -Werror -g -Og -fsanitize=thread -o $@ *.cpp

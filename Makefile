test: *.cpp *.h Makefile
	clang++ --stdlib=libc++ --std=c++20 -Wall -Wextra -pedantic -Werror -g -Og -fsanitize=thread -o $@ *.cpp

.PHONY: format
format:
	clang-format -i --style='{BasedOnStyle: Google, Language: Cpp, ColumnLimit: 80}' *.h *.cpp


clang-format -i $(find . \( -name \*.h -or -name \*.cpp \) -not -path "./build/*")
black $(find . -name \*.py -not -path "./build/*")

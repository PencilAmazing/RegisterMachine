##
# Register Machine
#
# @file
# @version 0.1

machine: ./machine.cpp ./machine.h
	${CXX} ./machine.cpp -Wall -Wextra -Werror -fno-exceptions -fno-rtti -o machine

test: machine
	./machine code.txt

# end

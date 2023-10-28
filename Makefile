##
# Register Machine
#
# @file
# @version 0.1

machine:
	${CXX} ./machine.cpp -Wall -Wextra -Werror -o machine

test: machine
	./machine code.txt

# end

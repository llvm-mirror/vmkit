#include <stdio.h>
#include "j3/j3.h"

int main(int argc, char** argv) {
	j3::J3* vm = j3::J3::create();

	vm->start(argc, argv);
}

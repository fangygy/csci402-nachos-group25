#include "syscall.h"

int main(){
	int i = 0;
	for(;i<9;i++){
		Exec("../test/passenger");
	}
}

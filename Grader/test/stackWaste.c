#include "syscall.h"

void wasteSpace(int loops){
      int wasteful[20];
      if(loops>10){
          Exit(0);
      }
      wasteSpace(++loops);
}

int
main()
{


	int loop = 0;
	wasteSpace(loop);

}

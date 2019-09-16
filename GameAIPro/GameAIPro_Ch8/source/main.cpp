#include "pch.h"
#include "jedi.h"

int main()
{
	// test the Jedi
	CJedi jedi;
	jedi.setup();
	while (true)
		jedi.process(0.333f);

	// done
	return 0;
}

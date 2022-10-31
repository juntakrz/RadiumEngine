#include "pch.h"
#include "core/core.h"

int wmain()
{
	// run application
  try {
    core::run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n.";
    _getch();
    return EXIT_FAILURE;
  };

  EXIT_SUCCESS;
}
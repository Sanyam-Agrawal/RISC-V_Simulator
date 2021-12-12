/* It contains the main driver's code for the simulator.
 *
 */
#include "Simulation.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: risc-v-sim <binary>\n";
    return 1;
  }

  MainMemory mainMemory{};
  Cache cache{};
  Memory memory{&mainMemory, &cache};
  Simulation sim{memory, argv[1]};

  std::cout << "Beginning the simulation...\n\n";
  try {
    sim.simulate();
  } catch (std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
  }
}

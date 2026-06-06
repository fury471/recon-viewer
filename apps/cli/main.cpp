#include <iostream>
#include <string>
#include <vector>

void printUsage() {
    std::cout<<
        "recon-viewer\n\n"
        "Usage:\n"
        "  cli --help              Show this help\n"
        "  cli reconstruct <dir>   Reconstruct a model from images (coming in Phase 2)\n";
}

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);   // all args after the program name

    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        printUsage();
        return 0;
    }

    std::cout << "Unknown command: " << args[0] << "\n\n";
    printUsage();
    return 1;
}
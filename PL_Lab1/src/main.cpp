#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/dot_export.h"
#include "../include/json_export.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

static const size_t READ_BLOCK_SIZE = 4096;

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open input file: " + path);
    }

    std::string content;
    char buffer[READ_BLOCK_SIZE];
    while (in.read(buffer, READ_BLOCK_SIZE)) {
        content.append(buffer, in.gcount());
    }
    content.append(buffer, in.gcount());
    return content;
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open output file: " + path);
    }

    size_t written = 0;
    while (written < content.size()) {
        size_t chunk = std::min(READ_BLOCK_SIZE, content.size() - written);
        out.write(content.data() + written, static_cast<std::streamsize>(chunk));
        written += chunk;
    }
}

static void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " [options] <input-file> <output-file>\n"
              << "\n"
              << "Options:\n"
              << "  --format=dot   Output in Graphviz DOT format (default)\n"
              << "  --format=json  Output in JSON format\n"
              << "\n"
              << "Parses a source file (Variant 4 language) and outputs the\n"
              << "syntax tree in the specified format.\n";
}

int main(int argc, char* argv[]) {
    enum Format { FMT_DOT, FMT_JSON };
    Format format = FMT_DOT;

    int argIdx = 1;
    while (argIdx < argc && argv[argIdx][0] == '-') {
        std::string arg = argv[argIdx];
        if (arg == "--format=dot") {
            format = FMT_DOT;
        } else if (arg == "--format=json") {
            format = FMT_JSON;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
        argIdx++;
    }

    if (argc - argIdx != 2) {
        printUsage(argv[0]);
        return 1;
    }

    const char* inputPath = argv[argIdx];
    const char* outputPath = argv[argIdx + 1];

    std::string source;
    try {
        source = readFile(inputPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    bool hasErrors = false;
    for (const auto& err : lexer.errors()) {
        std::cerr << inputPath << ":" << err.loc.line << ":" << err.loc.column
                  << ": lexer error: " << err.message << "\n";
        hasErrors = true;
    }

    Parser parser(tokens);
    ParseResult result = parser.parse();

    for (const auto& err : result.errors) {
        std::cerr << inputPath << ":" << err.loc.line << ":" << err.loc.column
                  << ": parse error: " << err.message << "\n";
        hasErrors = true;
    }

    if (result.tree) {
        std::string output;
        switch (format) {
            case FMT_DOT:
                output = DotExporter::exportTree(result.tree.get());
                break;
            case FMT_JSON:
                output = JsonExporter::exportTree(result.tree.get());
                break;
        }

        try {
            writeFile(outputPath, output);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
        std::cout << "Syntax tree written to " << outputPath << "\n";
    }

    return hasErrors ? 1 : 0;
}

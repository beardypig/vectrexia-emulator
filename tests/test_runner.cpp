#include <catch.hpp>
#include <trompeloeil.hpp>
#include <locale.h>

int main(int argc, char* argv[])
{
    // Set locale
    setlocale(LC_NUMERIC, "");

    // Initialize Catch2
    Catch::Session session;  // Catch2 session object

    // Pass command line arguments to Catch2
    int result = session.applyCommandLine(argc, argv);
    if (result != 0) {
        return result;  // Command-line error
    }

    // Run all tests
    return session.run();
}

#include <gtest/gtest.h>
#include <locale.h>

int main(int argc, char* argv[])
{
    setlocale(LC_NUMERIC, "");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

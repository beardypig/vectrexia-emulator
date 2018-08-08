#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <locale.h>

int main(int argc, char* argv[])
{
    setlocale(LC_NUMERIC, "");
    testing::InitGoogleTest(&argc, argv);
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

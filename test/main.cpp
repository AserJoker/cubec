#include <gtest/gtest.h>

extern "C" {
#include "core/icu_data.h"
}

int main(int argc, char **argv) {
  icu_data_init();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
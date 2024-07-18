#include "core/parameter/parameter_handle.h"
#include <gtest/gtest.h>

namespace aimrt::runtime::core::parameter {

class ParameterHandleTest : public ::testing::Test {
 protected:
  ParameterHandle parameter_handle_;
};

TEST_F(ParameterHandleTest, SetParameter) {
  EXPECT_EQ(parameter_handle_.GetParameter("test_parameter1"), nullptr);
  parameter_handle_.SetParameter("test_parameter1", "test_value1");
  EXPECT_EQ(*parameter_handle_.GetParameter("test_parameter1"), "test_value1");
}

TEST_F(ParameterHandleTest, ListParameter) {
  parameter_handle_.SetParameter("test_parameter1", "test_value1");
  parameter_handle_.SetParameter("test_parameter2", "test_value2");
  parameter_handle_.SetParameter("test_parameter3", "test_value3");
  EXPECT_EQ(parameter_handle_.ListParameter().size(), 3);
}

}  // namespace aimrt::runtime::core::parameter
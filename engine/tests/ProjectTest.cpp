#include <gtest/gtest.h>

#include <stdexcept>

#include "TestParts.hpp"
#include "embedded_market/model/Project.hpp"

using namespace em_test;

TEST(ProjectTest, FindsComponentsById) {
    Project project;
    project.addComponent(mcu3v3("MCU1").build());
    ASSERT_NE(project.findComponent("MCU1"), nullptr);
    EXPECT_EQ(project.findComponent("MCU1")->role(), ComponentRole::Controller);
    EXPECT_EQ(project.findComponent("nope"), nullptr);
}

TEST(ProjectTest, RejectsDuplicateComponentIds) {
    Project project;
    project.addComponent(mcu3v3("MCU1").build());
    EXPECT_THROW(project.addComponent(mcu3v3("MCU1").build()), std::invalid_argument);
}

TEST(ProjectTest, RejectsEmptyComponentId) {
    EXPECT_THROW(Component("", ComponentRole::Peripheral), std::invalid_argument);
}

TEST(ProjectTest, RejectsConnectionsToUnknownComponents) {
    Project project;
    project.addComponent(mcu3v3("MCU1").build());
    EXPECT_THROW(project.addConnection(Connection("MCU1", "U9", Interface::I2C)), std::invalid_argument);
    EXPECT_THROW(project.addConnection(Connection("U9", "MCU1", Interface::I2C)), std::invalid_argument);
}

TEST(ProjectTest, RejectsSelfConnections) {
    Project project;
    project.addComponent(mcu3v3("MCU1").build());
    EXPECT_THROW(project.addConnection(Connection("MCU1", "MCU1", Interface::I2C)), std::invalid_argument);
}

// Peripheral-to-peripheral chains (e.g. MCU -> I2C mux -> sensor) are plain connections.
TEST(ProjectTest, SupportsNestedPeripheralChains) {
    Project project;
    project.addComponent(mcu3v3("MCU1").build());
    project.addComponent(sensor3v3("MUX1", {Interface::I2C}).build());
    project.addComponent(sensor3v3("U1", {Interface::I2C}).build());
    project.addConnection(Connection("MCU1", "MUX1", Interface::I2C));
    project.addConnection(Connection("MUX1", "U1", Interface::I2C));

    ASSERT_EQ(project.downstreamOf("MCU1").size(), 1u);
    EXPECT_EQ(project.downstreamOf("MCU1")[0]->downstream(), "MUX1");
    ASSERT_EQ(project.downstreamOf("MUX1").size(), 1u);
    EXPECT_EQ(project.downstreamOf("MUX1")[0]->downstream(), "U1");
    ASSERT_EQ(project.upstreamOf("U1").size(), 1u);
    EXPECT_EQ(project.upstreamOf("U1")[0]->upstream(), "MUX1");
    EXPECT_TRUE(project.downstreamOf("U1").empty());
}

TEST(ProjectTest, ConnectionLabelNamesBothEndsAndInterface) {
    EXPECT_EQ(Connection("MCU1", "U2", Interface::SPI).label(), "MCU1 -> U2 (SPI)");
}

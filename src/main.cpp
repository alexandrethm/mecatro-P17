#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/loggers/bt_file_logger.h"
#include "dynamixel_sdk.h"
#include <wiringPi.h>
#include "loguru.hpp"

#include "components/AX12.h"
#include "components/Kangaroo.h"
#include "components/LCD.h"
#include "components/SerialPort.h"
#include "components/UltrasonicSensor.h"

#include "strategy/Nodes.h"

#define SENSOR_TRIGGER_PIN          23
#define SENSOR_ECHO_PIN_FRONT       24
#define SENSOR_ECHO_PIN_BACK        25


using namespace std;
using namespace BT;


int main(int argc, char *argv[]) {
    // -----------------------
    // Initialize logger
    loguru::init(argc, argv);
    loguru::add_file("../log/everything.log", loguru::Append, loguru::Verbosity_MAX);
    loguru::add_file("../log/info.log", loguru::Append, loguru::Verbosity_INFO);


    // -----------------------
    // Initialize robot components
    UltrasonicSensor frontSensor(SENSOR_TRIGGER_PIN, SENSOR_ECHO_PIN_FRONT);
    UltrasonicSensor backSensor(SENSOR_TRIGGER_PIN, SENSOR_ECHO_PIN_BACK);

    Kangaroo kangaroo(SERIAL_PORT_KANGAROO);
    if (kangaroo.isOperational())
        LOG_F(INFO, "Kangaroo is operational");
    else
        LOG_F(ERROR, "Kangaroo is not operational !");

    dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);
    dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);
    AX12 ax_1(1, portHandler, packetHandler);
    AX12 ax_4(4, portHandler, packetHandler);
    AX12 ax_5(5, portHandler, packetHandler);


    // -----------------------
    // Create the behavior tree

    // We use the BehaviorTreeFactory to register our custom nodes
    BehaviorTreeFactory factory;
    // Note: the name used to register should be the same used in the XML.
    using namespace Robot;

    // A node builder is nothing more than a function pointer to create a
    // std::unique_ptr<TreeNode>.
    // Using lambdas or std::bind, we can easily "inject" additional arguments.
    NodeBuilder builderMoveAhead;
    builderMoveAhead = [&](const std::string &name, const NodeConfiguration &config) {
        return std::make_unique<MoveAhead>(name, config, frontSensor, backSensor, kangaroo);
    };
    NodeBuilder builderTurn;
    builderTurn = [&](const std::string &name, const NodeConfiguration &config) {
        return std::make_unique<Turn>(name, config, kangaroo);
    };

    NodeBuilder builderMoveAX_1;
    builderMoveAX_1 = [&](const std::string &name, const NodeConfiguration &config) {
        return std::make_unique<MoveAX12>(name, config, ax_1);
    };
    NodeBuilder builderMoveAX_4;
    builderMoveAX_4 = [&](const std::string &name, const NodeConfiguration &config) {
        return std::make_unique<MoveAX12>(name, config, ax_4);
    };
    NodeBuilder builderMoveAX_5;
    builderMoveAX_5 = [&](const std::string &name, const NodeConfiguration &config) {
        return std::make_unique<MoveAX12>(name, config, ax_5);
    };

    factory.registerBuilder<MoveAhead>("MoveAhead", builderMoveAhead);
    factory.registerBuilder<MoveAX12>("MoveAX_1", builderMoveAX_1);
    factory.registerBuilder<MoveAX12>("MoveAX_4", builderMoveAX_4);
    factory.registerBuilder<MoveAX12>("MoveAX_5", builderMoveAX_5);


    // Trees are created at deployment-time (i.e. at run-time, but only
    // once at the beginning).

    // IMPORTANT: when the object "tree" goes out of scope, all the
    // TreeNodes are destroyed
    auto tree = factory.createTreeFromFile("/home/pi/mecatro_P17/src/strategy/tree_dev.xml"); // requires absolute paths

    // This logger saves state changes on file
    FileLogger logger_file(tree, "/home/pi/mecatro_P17/log/bt_trace.fbl");
    printTreeRecursively(tree.root_node);

    // -----------------------
    // Execute the behavior tree
    while (tree.root_node->executeTick() == NodeStatus::RUNNING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}
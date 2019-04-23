//
// Created by Alexandre Thomas on 2019-04-16.
//

#ifndef MECATRO_P17_DUMMY_NODES_H
#define MECATRO_P17_DUMMY_NODES_H

#include "behaviortree_cpp/behavior_tree.h"
#include "behaviortree_cpp/bt_factory.h"

namespace DummyNodes {
    void SleepMS(int x);

    BT::NodeStatus CheckBattery();

    class GripperInterface {
    public:
        GripperInterface() : _opened(true) {
        }

        BT::NodeStatus open();

        BT::NodeStatus close();

    private:
        bool _opened;
    };

//--------------------------------------

// Example of custom SyncActionNode (synchronous action)
// without ports.
    class ApproachObject : public BT::SyncActionNode {
    public:
        ApproachObject(const std::string &name) :
                BT::SyncActionNode(name, {}) {
        }

        // You must override the virtual function tick()
        BT::NodeStatus tick() override;
    };

// Example of custom SyncActionNode (synchronous action)
// with an input port.
    class SaySomething : public BT::SyncActionNode {
    public:
        SaySomething(const std::string &name, const BT::NodeConfiguration &config)
                : BT::SyncActionNode(name, config) {
        }

        // You must override the virtual function tick()
        BT::NodeStatus tick() override;

        // It is mandatory to define this static method.
        static BT::PortsList providedPorts() {
            return {BT::InputPort<std::string>("message")};
        }
    };

//Same as class SaySomething, but to be registered with SimpleActionNode
    BT::NodeStatus SaySomethingSimple(BT::TreeNode &self);


    inline void RegisterNodes(BT::BehaviorTreeFactory &factory) {
        static GripperInterface grip_singleton;

        factory.registerSimpleCondition("CheckBattery", std::bind(CheckBattery));
        factory.registerSimpleAction("OpenGripper", std::bind(&GripperInterface::open, &grip_singleton));
        factory.registerSimpleAction("CloseGripper", std::bind(&GripperInterface::close, &grip_singleton));
        factory.registerNodeType<ApproachObject>("ApproachObject");
        factory.registerNodeType<SaySomething>("SaySomething");
    }

    // Custom type
    struct Pose2D {
        double x, y, theta;
    };


    class MoveBaseAction : public BT::AsyncActionNode {
    public:
        MoveBaseAction(const std::string &name, const BT::NodeConfiguration &config)
                : AsyncActionNode(name, config) {}

        static BT::PortsList providedPorts() {
            return {BT::InputPort<Pose2D>("goal")};
        }

        BT::NodeStatus tick() override;

        // This overloaded method is used to stop the execution of this node.
        void halt() override {
            _halt_requested.store(true);
        }

    private:
        std::atomic_bool _halt_requested;
    };


} // end namespace

#endif //MECATRO_P17_DUMMY_NODES_H

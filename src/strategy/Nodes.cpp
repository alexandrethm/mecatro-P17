//
// Created by Alexandre Thomas on 2019-05-03.
//

#include "Nodes.h"

// ------ Kangaroo -------
NodeStatus Robot::MoveAhead::tick() {
    // ------------------------
    // Read move instructions from the blackboard
    auto moveForwardInput = getInput<bool>("moveForward");
    auto distanceInput = getInput<int>("distance");
    if (!moveForwardInput) {
        throw BT::RuntimeError("missing required input [moveForward]: ",
                               moveForwardInput.error());
    }
    if (!distanceInput) {
        throw BT::RuntimeError("missing required input [distance]: ",
                               distanceInput.error());
    }
    int distanceToTravelMm = distanceInput.value();
    bool moveForward = moveForwardInput.value();
    if (!moveForward)
        distanceToTravelMm *= -1;

    // Use the appropriate ultrasonic sensor to check for obstacles
    UltrasonicSensor &sensor = moveForward ? frontSensor : backSensor;


    // ------------------------
    // Move or wait logic
    bool isMoveCompleted = false;
    int distanceToTravelUnits = (int) (distanceToTravelMm * UNITS_PER_MM);
    int distanceTraveledUnits = 0;

    while (!isMoveCompleted) {
        // Check for any obstacles ahead
        int distanceToObstacle = sensor.getDistance();
        // if we have an error code (distance < 0), we consider there is no obstacle
        // (note that ahead can mean front or back, depending on the current move direction)
        bool obstacleAhead = (distanceToObstacle > 0 and distanceToObstacle < SENSOR_OBSTACLE_THRESHOLD);

        if (isMoving and !obstacleAhead) {
            if (kangaroo.isMoveCompleted()) {
                isMoveCompleted = true;
                LOG_F(INFO, "straight move completed (total distance : %dmm, %d units)", distanceToTravelMm,
                      distanceToTravelUnits);
            } else {
                LOG_F(1, "straight move going on, no obstacle");
                setStatusRunningAndYield();
            }
        } else {
            if (isMoving) {
                // Moving with obstacle ahead : stop movement and keep in memory current position
                distanceTraveledUnits = kangaroo.getPosition();
                kangaroo.startStraightMove(0, 0); //todo: check if command powerdown is more appropriate
                isMoving = false;
                LOG_F(INFO, "obstacle ahead, stopped movement at distance %d", distanceTraveledUnits);
            } else if (!obstacleAhead) {
                // Idle with no obstacle ahead : start/resume movement
                kangaroo.startStraightMove(distanceToTravelUnits - distanceTraveledUnits, DEFAULT_KANGAROO_SPEED);
                isMoving = true;
            }
            setStatusRunningAndYield();
        }
    }

    cleanup(false);
    return NodeStatus::SUCCESS;
}

void Robot::MoveAhead::cleanup(bool halted) {

}

void Robot::MoveAhead::halt() {
    //todo: stop movement ?
    cleanup(true);
    // Do not forget to call this at the end.
    CoroActionNode::halt();
}

NodeStatus Robot::Turn::tick() {
    // ------------------------
    // Read move instructions from the blackboard
    cout << "ok5" << endl;
    auto angleInput = getInput<int>("angle");
    if (!angleInput) {
        throw BT::RuntimeError("missing required input [angle]: ",
                               angleInput.error());
    }
    int angle = angleInput.value();
    // faire la conversion
    int angleUnits = angle;

    // ------------------------
    // Move or wait logic
    bool isTurnCompleted = false;
    cout << "ok6" << endl;
    cout << kangaroo.isMoveCompleted() << endl;
    while (!kangaroo.isMoveCompleted()) {
        cout << "llll" << endl;
        LOG_F(1, "turning... ");
        kangaroo.startTurnMove(angleUnits, DEFAULT_KANGAROO_ROTATION_SPEED);
        setStatusRunningAndYield();
    }
    LOG_F(INFO, "straight move completed (total distance : %d degrés, %d units)", angle, angleUnits);

    cleanup(false);
    return NodeStatus::SUCCESS;
}

void Robot::Turn::cleanup(bool halted) {

}

void Robot::Turn::halt() {
    //todo: stop movement ?
    cleanup(true);
    // Do not forget to call this at the end.
    CoroActionNode::halt();
}

// ------ AX12 -------
NodeStatus Robot::MoveAX12Joint::tick() {
    // ------------------------
    // Read move instructions from the blackboard
    auto positionInput = getInput<int>("pos");
    if (!positionInput) {
        throw BT::RuntimeError("missing required input [pos]: ",
                               positionInput.error());
    }
    auto goalPosition = positionInput.value();

    // ------------------------
    // Move logic
    _ax.setMode(joint);
    _ax.setSpeed(0); // 0 : maximum speed (same as 1023)
    _ax.goToPositionJointMode(goalPosition);

    auto currentPosition = _ax.getPosition();
    while (std::abs(goalPosition - currentPosition) > DXL_MOVING_STATUS_THRESHOLD) {
        currentPosition = _ax.getPosition();
        LOG_F(INFO, "[id %d] moving ; GoalPos:%03d CurrentPos:%03d", _ax.ID, goalPosition, currentPosition);
        setStatusRunningAndYield();
    }

    cleanup(false);
    return NodeStatus::SUCCESS;
}

void Robot::MoveAX12Joint::cleanup(bool halted) {
}

void Robot::MoveAX12Joint::halt() {
    CoroActionNode::halt();
}

NodeStatus Robot::MoveAX12Wheel::tick() {
    // Get inputs from the blackboard
    auto durationInput = getInput<int>("duration");
    if (!durationInput) {
        throw BT::RuntimeError("missing required input [duration]: ",
                               durationInput.error());
    }
    auto duration = std::chrono::milliseconds(durationInput.value());

    auto isCWInput = getInput<bool>("isCW");
    if (!isCWInput) {
        throw BT::RuntimeError("missing required input [isCW]: ",
                               isCWInput.error());
    }
    auto isCW = isCWInput.value();

    // Rotation direction (CW or CCW) is controlled by the 10th bit
    int speed = isCW ? DEFAULT_AX12_WHEEL_SPEED + 1024 : DEFAULT_AX12_WHEEL_SPEED;
    int zero = isCW ? 1024 : 0;

    // Start the movement
    _ax.setMode(wheel);
    auto initialTime = std::chrono::system_clock::now();
    _ax.setSpeed(speed);

    auto currentPosition = _ax.getPosition();
    auto currentTime = std::chrono::system_clock::now();
    while (currentTime < initialTime + duration) {
        currentPosition = _ax.getPosition();
        currentTime = std::chrono::system_clock::now();
        LOG_F(INFO, "[id %d] moving ; GoalDuration:%03d CurrentDuration:%03d CurrentPos:%03d", _ax.ID,
              duration.count(), (currentTime - initialTime).count(), currentPosition);
        setStatusRunningAndYield();
    }

    // Stop the movement
    _ax.setSpeed(zero);


    cleanup(false);
    return NodeStatus::SUCCESS;
}

void Robot::MoveAX12Wheel::cleanup(bool halted) {

}

void Robot::MoveAX12Wheel::halt() {
    CoroActionNode::halt();
}

// ------ Other -------
NodeStatus Robot::UpdateScore::tick() {
    auto pointsEarnedInput = getInput<int>("pointsEarned");
    if (!pointsEarnedInput) {
        throw BT::RuntimeError("missing required input [pointsEarned]: ",
                               pointsEarnedInput.error());
    }
    int pointsEarned = pointsEarnedInput.value();
    currentScore += pointsEarned;

    lcd.printToScreenCentered("Score : " + std::to_string(currentScore));
    LOG_F(INFO, "Added %d to the score. New current score :%d", pointsEarned, currentScore);

    return NodeStatus::SUCCESS;
}
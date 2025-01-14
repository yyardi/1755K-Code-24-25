#include "main.h"
#include "lemlib/api.hpp" // IWYU pragma: keep
#include "lemlib/chassis/trackingWheel.hpp"
#include "pros/adi.h"
#include "pros/adi.hpp"
#include "pros/misc.h"
#include "pros/motors.h"
#include <atomic>
#include "autons.hpp"

//electronics variables
bool isClamp = false;
bool clampLatch = false;

bool isDoinker = false;
bool doinkerLatch = false;

int currentPositionIndex = 0;
bool lastCycleButtonState = false;

const int lbDown = 0;
const int lbMid = 420;
const int lbScore = 2000;
const int positions[] = {lbDown, lbMid, lbScore};

bool isColorSortEnabled = true;  // Start enabled by default
std::atomic<bool> isRedTeam(true); // Atomic for thread safety
pros::Task* colorSortTask = nullptr;

// electronics declarations
pros::Controller controller(pros::E_CONTROLLER_MASTER);

pros::MotorGroup leftMotors({-18, -19, -20}, pros::MotorGearset::blue); // left motor group - reversed
pros::MotorGroup rightMotors({8, 9, 10}, pros::MotorGearset::blue); // right motor group -

pros::Imu imu(17);

// tracking wheels
// horizontal tracking wheel encoder. Rotation sensor, not reversed
pros::Rotation horizontalEnc(20);
// vertical tracking wheel encoder. Rotation sensor, reversed
pros::Rotation verticalEnc(-11);

// horizontal tracking wheel. 2" diameter, 5.75" offset, back of the robot (negative)
lemlib::TrackingWheel horizontal(&horizontalEnc, lemlib::Omniwheel::NEW_2, -5.75);
// vertical tracking wheel. 2" diameter, 2.5" offset, left of the robot (negative)
lemlib::TrackingWheel vertical(&verticalEnc, lemlib::Omniwheel::NEW_2, -2.5);

// drivetrain settings
lemlib::Drivetrain drivetrain(&leftMotors, // left motor group
                              &rightMotors, // right motor group
                              13.5, // track width
                              lemlib::Omniwheel::NEW_275, // using new 2.75" omnis
                              450, // drivetrain rpm is 360
                              2 // horizontal drift is 2. If we had traction wheels, it would have been 8
);

// lateral motion controller
lemlib::ControllerSettings linearController(10, // proportional gain (kP)
                                            0, // integral gain (kI)
                                            3, // derivative gain (kD)
                                            3, // anti windup
                                            1, // small error range, in inches
                                            100, // small error range timeout, in milliseconds
                                            3, // large error range, in inches
                                            500, // large error range timeout, in milliseconds
                                            20 // maximum acceleration (slew)
);

// angular motion controller
lemlib::ControllerSettings angularController(2, // proportional gain (kP)
                                             0, // integral gain (kI)
                                             10, // derivative gain (kD)
                                             3, // anti windup
                                             1, // small error range, in degrees
                                             100, // small error range timeout, in milliseconds
                                             3, // large error range, in degrees
                                             500, // large error range timeout, in milliseconds
                                             0 // maximum acceleration (slew)
);

// sensors for odometry
lemlib::OdomSensors sensors(&vertical, // vertical tracking wheel
                            nullptr, // vertical tracking wheel 2, set to nullptr as we don't have a second one
                            &horizontal, // horizontal tracking wheel
                            nullptr, // horizontal tracking wheel 2, set to nullptr as we don't have a second one
                            &imu // inertial sensor
);

// input curve for throttle input during driver control
lemlib::ExpoDriveCurve throttleCurve(3, // joystick deadband out of 127
                                     6, // minimum output where drivetrain will move out of 127
                                     1.019 // expo curve gain
);

// input curve for steer input during driver control
lemlib::ExpoDriveCurve steerCurve(3, // joystick deadband out of 127
                                  6, // minimum output where drivetrain will move out of 127
                                  1.019 // expo curve gain
);

// create the chassis
lemlib::Chassis chassis(drivetrain, linearController, angularController, sensors, &throttleCurve, &steerCurve);

pros::Motor intakeLow(-11);
pros::Motor intakeHigh(-7);

pros::Optical colorsort(2); //change port


pros::Motor ladybrown(5);
pros::ADIDigitalOut doinker('A');
pros::ADIDigitalOut mogoclamp('C');

void sorting() {
    while (true) {
        if (isColorSortEnabled) {
            auto values = colorsort.get_rgb();
            bool bad_ring_detected;
            
            if (isRedTeam.load()) {  // check team color multithread
                bad_ring_detected = values.blue > 220 && values.red < 220; //red team
                if (bad_ring_detected) {
                    intakeHigh.move(127); // Fling off wrong color
                    pros::delay(200);
                    intakeHigh.move(0);
                }
            } 
            else {
                bad_ring_detected = values.blue < 220 && values.red > 220; //blue team
                if (bad_ring_detected) {
                    intakeHigh.move(127); // Fling off wrong color
                    pros::delay(200);
                    intakeHigh.move(0);
                }
            }
            
            // Update LED based on sorting status
            colorsort.set_led_pwm(100); //if color sort on then led on
        } 
        else {
            // Turn off LED when sorting is disabled
            colorsort.set_led_pwm(0);
        }
        
        pros::delay(20); // Small delay to prevent killing our CPU
    }
}

//use these with the autons selector
void selectRedTeam() {
    isRedTeam.store(true);
}

void selectBlueTeam() {
    isRedTeam.store(false);
}



/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
    pros::lcd::initialize(); // initialize brain screen
    chassis.calibrate(); // calibrate sensors
    // thread to for brain screen and position logging
    colorSortTask = new pros::Task(sorting);
    
    AutonSelector::getInstance().init();
    
    // task for updating display
    pros::Task displayTask([&]() {
        while (true) {
            AutonSelector::getInstance().update();
            pros::delay(50);
        }
    });

    // pros::Task screenTask([&]() {
    //     while (true) {
    //         // print robot location to the brain screen
    //         pros::lcd::print(0, "X: %f", chassis.getPose().x); // x
    //         pros::lcd::print(1, "Y: %f", chassis.getPose().y); // y
    //         pros::lcd::print(2, "Theta: %f", chassis.getPose().theta); // heading
    //         // log position telemetry
    //         lemlib::telemetrySink()->info("Chassis pose: {}", chassis.getPose());
    //         // delay to save resources
    //         pros::delay(50);
    //     }
    // });
}

/**
 * Runs while the robot is disabled
 */
void disabled() {}

/**
 * runs after initialize if the robot is connected to field control
 */
void competition_initialize() {}

// get a path used for pure pursuit
// in the static folder 
ASSET(example_txt); // '.' replaced with "_" to make c++ happy 




/**
 * Runs during auto
 *
 * This is an example autonomous routine which demonstrates a lot of the features LemLib has to offer
 */
void autonomous() {
	chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    ladybrown.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);  	
  	doinker.set_value(LOW);
  	mogoclamp.set_value(LOW);
	isColorSortEnabled = true; //enable color sort for all of auto -- we could cook on the corners??

    AutonSelector::getInstance().runSelectedAuton();

    
}

/**
 * Runs in driver control
 */
void opcontrol() {
	chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST);
	ladybrown.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    isColorSortEnabled = true; //start with color sort on

	while (true) {
        if (!pros::competition::is_connected()) {
            if (controller.get_digital(DIGITAL_B) && 
                controller.get_digital(DIGITAL_DOWN)) {
                autonomous(); //runs auton
                chassis.setBrakeMode(pros::E_MOTOR_BRAKE_COAST); //when done go back to coast for driver
            }
        }

        //intake 
        if (controller.get_digital(DIGITAL_R1)) {
            intakeLow.move(127);
            intakeHigh.move(113);
        } 
        else if (controller.get_digital(DIGITAL_R2)) {
            intakeLow.move(-127);
            intakeHigh.move(-113);
        } 
        else {
            intakeLow.move(0);
            intakeHigh.move(0);
        }   

        //mogo
        if (isClamp){
            mogoclamp.set_value(HIGH);
        } 
        else {
            mogoclamp.set_value(LOW);
        }
        if (controller.get_digital(DIGITAL_L2)) {
            if (!clampLatch) {
                isClamp = !isClamp;
                clampLatch = true;
            } 
        }    
        else {
            clampLatch = false;
        }

        //doinker
        if (isDoinker){
            doinker.set_value(HIGH);
        } 
        else {
            doinker.set_value(LOW);
        }
        if (controller.get_digital(DIGITAL_L1)) {
            if (!doinkerLatch) {
                isDoinker = !isDoinker;
                doinkerLatch = true;
            } 
        }    
        else {
            doinkerLatch = false;
        }

        // ladybrown
        if (controller.get_digital(DIGITAL_DOWN)) {
            // currentPositionIndex = (currentPositionIndex + 1) % 3;
            ladybrown.move_absolute(0, 127);
            // ladyBrownAngle(positions[currentPositionIndex]);
        }

        if (controller.get_digital(DIGITAL_UP)) {
            // currentPositionIndex = 0;
            ladybrown.move_absolute(1850, 127);

            // ladyBrownAngle(positions[0]);
            // ladybrown.tare_position();
        }

        if (controller.get_digital(DIGITAL_LEFT)) {
            // currentPositionIndex = 0;
            ladybrown.move_absolute(380, 127);
            intakeHigh.move_relative(200, -127); //need to get a super short outtake
        }



        //color sort
        if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_Y)) {
            isColorSortEnabled = true;
        }
        else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_X)) {
            isColorSortEnabled = false;
        }
        else {
            isColorSortEnabled = true;
        }

          


        // arcade control
        int leftY = controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
        int rightX = controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
        // move the chassis with curvature drive
        chassis.arcade(leftY, rightX);
        // delay to save resources
        pros::delay(10);

		// //other Arcade control scheme
		// int dir = controller.get_analog(ANALOG_LEFT_Y);    // Gets amount forward/backward from left joystick
		// int turn = controller.get_analog(ANALOG_RIGHT_X);  // Gets the turn left/right from right joystick
		// leftMotors.move(dir - turn);                      // Sets left motor voltage
		// rightMotors.move(dir + turn);                     // Sets right motor voltage
		// pros::delay(20);                               // Run for 20 ms then update
	}
}
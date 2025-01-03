#include "vex.h"

using namespace vex;
using signature = vision::signature;
using code = vision::code;

// A global instance of brain used for printing to the V5 Brain screen
brain  Brain;

// VEXcode device constructors
controller Controller1 = controller(primary);
motor IntakeMotorA = motor(PORT5, ratio6_1, false);
motor IntakeMotorB = motor(PORT6, ratio6_1, true);
motor_group Intake = motor_group(IntakeMotorA, IntakeMotorB);
digital_out Clamp = digital_out(Brain.ThreeWirePort.A);
motor LF = motor(PORT1, ratio6_1, true);
motor RF = motor(PORT2, ratio6_1, false);
motor LB = motor(PORT3, ratio6_1, true);
motor RB = motor(PORT4, ratio6_1, false);

// VEXcode generated functions
// define variable for remote controller enable/disable
bool RemoteControlCodeEnabled = true;

/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 * 
 * This should be called at the start of your int main function.
 */
void vexcodeInit( void ) {
  // nothing to initialize
}
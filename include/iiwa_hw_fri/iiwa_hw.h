/*
* This class implements a bridge between ROS hardware interfaces and a KUKA LBR IIWA Robot,
* using KUKA Sunrise.FRI and iiwa_stack repo.
*
* It combines multiple works from
* https://github.com/RobotLocomotion/drake-iiwa-driver.git
* https://github.com/SalvoVirga/iiwa_stack.git
* https://github.com/ahundt/grl
*
* We acknowledge the good work of the prior contributors :
* Salvatore Virga - salvo.virga@tum.de, Marco Esposito - marco.esposito@tum.de
* Andrew Hundt -
* Carlos J. Rosales - cjrosales@gmail.com
* Enrico Corvaglia
* Marco Esposito - marco.esposito@tum.de
* Manuel Bonilla - josemanuelbonilla@gmail.com
*
* LICENSE :
* Copyright (c) <2018>, <Dinesh Thakur>
* All rights reserved.
*
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   1. Redistributions of source code must retain the above copyright notice,
*   this list of conditions and the following disclaimer.
*
*   2. Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
*   3. Neither the name of the University of Pennsylvania nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

// iiwa_msgs and ROS inteface includes
#include <iiwa_msgs/JointPosition.h>
#include <iiwa_msgs/JointVelocity.h>
#include <iiwa_msgs/JointTorque.h>
#include <iiwa_msgs/ControlMode.h>
#include <iiwa_msgs/ConfigureSmartServo.h>

// ROS headers
#include <controller_manager/controller_manager.h>
#include <control_toolbox/filters.h>
#include <hardware_interface/joint_state_interface.h>
#include <hardware_interface/joint_command_interface.h>
#include <joint_limits_interface/joint_limits.h>
#include <joint_limits_interface/joint_limits_interface.h>
#include <joint_limits_interface/joint_limits_rosparam.h>
#include <joint_limits_interface/joint_limits_urdf.h>
#include <std_msgs/Duration.h>
#include <ros/ros.h>
#include <urdf/model.h>
#include <sensor_msgs/JointState.h>

#include <vector>
#include <sstream>

#include "friClientApplication.h"
#include "friLBRClient.h"
#include "friUdpConnection.h"

constexpr int DEFAULT_CONTROL_FREQUENCY = 1000; // Hz
constexpr int IIWA_JOINTS = 7;

  /** Structure for a lbr iiwa, joint handles, etc */
  struct IIWA_device {
      /** Vector containing the name of the joints - taken from yaml file */
      std::vector<std::string> joint_names;

      std::vector<double>
      joint_lower_limits, /**< Lower joint limits */
      joint_upper_limits, /**< Upper joint limits */
      joint_lower_soft_limits, /**< Lower joint limits */
      joint_upper_soft_limits, /**< Upper joint limits */
      joint_effort_limits, /**< Effort joint limits */
      joint_velocity_limits; /**< Velocity joint limits */

      /**< Joint state and commands */
      std::vector<double>
      joint_position,
      joint_position_prev,
      joint_velocity,
      joint_effort,
      joint_position_command,
      joint_stiffness_command,
      joint_damping_command,
      joint_effort_command,
      joint_velocity_command;

      /**
       * \brief Initialze vectors
       */
      void init() {
          joint_position.resize(IIWA_JOINTS);
          joint_position_prev.resize(IIWA_JOINTS);
          joint_velocity.resize(IIWA_JOINTS);
          joint_effort.resize(IIWA_JOINTS);
          joint_position_command.resize(IIWA_JOINTS);
          joint_effort_command.resize(IIWA_JOINTS);
          joint_stiffness_command.resize(IIWA_JOINTS);
          joint_damping_command.resize(IIWA_JOINTS);
          joint_velocity_command.resize(IIWA_JOINTS);

          joint_lower_limits.resize(IIWA_JOINTS);
          joint_upper_limits.resize(IIWA_JOINTS);
          joint_lower_soft_limits.resize(IIWA_JOINTS);
          joint_upper_soft_limits.resize(IIWA_JOINTS);
          joint_effort_limits.resize(IIWA_JOINTS);
          joint_velocity_limits.resize(IIWA_JOINTS);
      }

      /**
       * \brief Reset values of the vectors
       */
      void reset() {
          for (int j = 0; j < IIWA_JOINTS; ++j) {
              joint_position[j] = 0.0;
              joint_position_prev[j] = 0.0;
              joint_velocity[j] = 0.0;
              joint_effort[j] = 0.0;
              joint_position_command[j] = 0.0;
              joint_effort_command[j] = 0.0;
              joint_velocity_command[j] = 0.0;

              // set default values for these two for now
              joint_stiffness_command[j] = 0.0;
              joint_damping_command[j] = 0.0;
          }
      }
  };

class KukaFRIClient : public KUKA::FRI::LBRClient
{

public:

  KukaFRIClient(boost::shared_ptr<IIWA_device> device);

  ~KukaFRIClient();

  enum CommandType {
    Position,
    Velocity,
    Effort
  };

  /**
  * \brief Callback for FRI state changes.
  *
  * @param oldState
  * @param newState
  */
  virtual void onStateChange(KUKA::FRI::ESessionState oldState, KUKA::FRI::ESessionState newState);

  /**
  * \brief Callback for the FRI session states 'Monitoring Wait' and 'Monitoring Ready'.
  *
  * If you do not want to change the default-behavior, you do not have to implement this method.
  */
  virtual void monitor();

  /**
  * \brief Callback for the FRI session state 'Commanding Wait'.
  *
  * If you do not want to change the default-behavior, you do not have to implement this method.
  */
  virtual void waitForCommand();

  /**
  * \brief Callback for the FRI state 'Commanding Active'.
  *
  * If you do not want to change the default-behavior, you do not have to implement this method.
  */
  virtual void command();

  void setCommandValid(CommandType command_type);

private:
  double ToRadians(double degrees);
  void ApplyJointPosLimits(double* pos);
  void UpdateRobotState(const KUKA::FRI::LBRState& state);

  std::vector<double> joint_limits_;

  // What was the joint position when we entered command state?
  // (provided so that we can keep holding that position).
  std::vector<double> joint_position_when_command_entered_;
  bool has_entered_command_state_;
  bool inhibit_motion_in_command_state_;

  bool command_valid_;
  std::vector<double> last_joint_position_command_;

  int64_t utime_last_read_, utime_last_control_;
  boost::shared_ptr<IIWA_device> device_;
  CommandType command_type_;
  bool once_;
};

class IIWA_HW : public hardware_interface::RobotHW {
public:

    enum ParamIndex {
        RobotName,
        RobotModel,
        LocalUDPAddress,
        LocalUDPPort,
        RemoteUDPAddress,
        LocalHostKukaKoniUDPAddress,
        LocalHostKukaKoniUDPPort,
        RemoteHostKukaKoniUDPAddress,
        RemoteHostKukaKoniUDPPort,
        KukaCommandMode,
        KukaMonitorMode
    };

    typedef std::tuple<
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string,
        std::string
    > Params;

    /**
     * Constructor
     */
    IIWA_HW(ros::NodeHandle nh);

    /**
     * Destructor
     */
    virtual ~IIWA_HW();

    /**
     * \brief Initializes the IIWA device struct and all the hardware and joint limits interfaces needed.
     *
     * A joint state handle is created and linked to the current joint state of the IIWA robot.
     * A joint position handle is created and linked  to the command joint position to send to the robot.
     */
    bool start();

    /**
     * \brief Registers the limits of the joint specified by joint_name and joint_handle.
     *
     * The limits are retrieved from the urdf_model.
     * Returns the joint's type, lower position limit, upper position limit, and effort limit.
     */
    void registerJointLimits(const std::string& joint_name,
                             const hardware_interface::JointHandle& position_joint_handle,
                             const hardware_interface::JointHandle& effort_joint_handle,
                             const urdf::Model *const urdf_model,
                             double *const lower_limit, double *const upper_limit,
                             double *const lower_soft_limit, double *const upper_soft_limit,
                             double *const effort_limit);

    void registerVelocityJointLimits(const std::string& joint_name,
                                     const hardware_interface::JointHandle& velocity_joint_handle,
                                     const urdf::Model *const urdf_model,
                                     double *const velocity_limit);

    /**
     * \brief Reads the current robot state via the IIWARos interfae and sends the values to the IIWA device struct.
     */
    bool read(ros::Duration period);

    /**
     * \brief Sends the command joint position to the robot via IIWARos interface
     */
    bool write(ros::Duration period);

    /**
     * \brief Retuns the ros::Rate object to control the receiving/sending rate.
     */
    ros::Rate* getRate();

    /**
     * \brief Retuns the current frequency used by a ros::Rate object to control the receiving/sending rate.
     */
    double getFrequency();

    /**
     * \brief Set the frequency to be used by a ros::Rate object to control the receiving/sending rate.
     */
    void setFrequency(double frequency);

    bool prepareSwitch(const std::list<hardware_interface::ControllerInfo>& start_list,
                             const std::list<hardware_interface::ControllerInfo>& stop_list);

private:

    /* Node handle */
    ros::NodeHandle nh_;

    /* Parameters */
    std::string interface_, movegroup_name_;
    urdf::Model urdf_model_;

    hardware_interface::JointStateInterface state_interface_; /**< Interface for joint state */
    hardware_interface::EffortJointInterface effort_interface_; /**< Interface for joint impedance control */
    hardware_interface::PositionJointInterface position_interface_; /**< Interface for joint position control */
    hardware_interface::VelocityJointInterface velocity_interface_; /**< Interface for joint velocity control */

    /** Interfaces for limits */
    joint_limits_interface::EffortJointSaturationInterface   ej_sat_interface_;
    joint_limits_interface::EffortJointSoftLimitsInterface   ej_limits_interface_;
    joint_limits_interface::PositionJointSaturationInterface pj_sat_interface_;
    joint_limits_interface::PositionJointSoftLimitsInterface pj_limits_interface_;
    joint_limits_interface::VelocityJointSaturationInterface vj_sat_interface_;
    joint_limits_interface::VelocityJointSoftLimitsInterface vj_limits_interface_;

    boost::shared_ptr<IIWA_device> device_; /**< IIWA device. */

    /** Objects to control send/receive rate. */
    ros::Time timer_;
    ros::Rate* loop_rate_;
    double control_frequency_;

    iiwa_msgs::JointPosition joint_position_;
    iiwa_msgs::JointTorque joint_torque_;

    iiwa_msgs::JointPosition command_joint_position_;
    iiwa_msgs::JointTorque command_joint_torque_;
    iiwa_msgs::JointVelocity command_joint_velocity_;

    std::vector<double> last_joint_position_command_;

    std::vector<std::string> interface_type_; /**< Contains the strings defining the possible hardware interfaces. */

    boost::mutex jt_mutex;
    Params params_;

    ros::Publisher js_pub_; // publish true joint states from the KUKA
    ros::Publisher wrench_pub_;

    sensor_msgs::JointState current_js_;

    std::unique_ptr<KukaFRIClient> fri_client_;
    std::unique_ptr<KUKA::FRI::ClientApplication> client_app_;
    std::unique_ptr<KUKA::FRI::UdpConnection> udp_connection_;

    bool ready_for_command_;
    ros::Time ready_for_command_timer_;
    ros::Duration ignore_command_duration_; //Amount of time to ignore write commands after controller init/switch
};

template <typename T>
void iiwaMsgsJointToVector(const iiwa_msgs::JointQuantity& ax, std::vector<T>& v) {
    v[0] = ax.a1;
    v[1] = ax.a2;
    v[2] = ax.a3;
    v[3] = ax.a4;
    v[4] = ax.a5;
    v[5] = ax.a6;
    v[6] = ax.a7;
}

template <typename T>
void vectorToIiwaMsgsJoint(const std::vector<T>& v, iiwa_msgs::JointQuantity& ax) {
    ax.a1 = v[0];
    ax.a2 = v[1];
    ax.a3 = v[2];
    ax.a4 = v[3];
    ax.a5 = v[4];
    ax.a6 = v[5];
    ax.a7 = v[6];
}

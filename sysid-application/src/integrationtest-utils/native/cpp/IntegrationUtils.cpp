// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "IntegrationUtils.h"

#include <stdexcept>

#include <fmt/core.h>
#include <wpi/timestamp.h>

#include "gtest/gtest.h"

std::shared_ptr<wpi::uv::Process> LaunchSim(std::string_view projectDirectory, wpi::uv::Pipe &pipe) {
  // Install the robot program.
  std::string installCmd =
      fmt::format("cd {}/ && {} :{}:installSimulateNativeRelease -Pintegration",
                  EXPAND_STRINGIZE(PROJECT_ROOT_DIR), LAUNCH, projectDirectory);
  fmt::print(stderr, "Executing: {}\n", installCmd);

  auto build = wpi::uv::Process::SpawnArray(wpi::uv::Loop::GetDefault(),
    LAUNCH, 
    {
      fmt::format(":{}:installSimulateNativeRelease", projectDirectory),
      "-Pintegration",
      wpi::uv::Process::Cwd(EXPAND_STRINGIZE(PROJECT_ROOT_DIR))
    });

  wpi::uv::Loop::GetDefault()->Run();

  // TODO: Return code?
  // auto result = ???

  // // Exit the test if we could not install the robot program.
  // if (result != 0) {
  //   fmt::print(stderr, "The robot program could not be installed.\n");
  //   std::exit(1);
  // }

  // Run the robot program.
  std::string runCmd =
      fmt::format("cd {}/ && {} :{}:simulateNativeRelease -Pintegration {}",
                  EXPAND_STRINGIZE(PROJECT_ROOT_DIR), LAUNCH_DETACHED,
                  projectDirectory, DETACHED_SUFFIX);
  fmt::print(stderr, "Executing: {}\n", runCmd);

  auto process = wpi::uv::Process::SpawnArray(wpi::uv::Loop::GetDefault(),
    LAUNCH,
    {
      fmt::format(":{}:simulateNativeRelease", projectDirectory),
      "-Pintegration",
      wpi::uv::Process::Cwd(EXPAND_STRINGIZE(PROJECT_ROOT_DIR)),
      wpi::uv::Process::StdioInherit(1, pipe)
    });

  wpi::uv::Loop::GetDefault()->Run(wpi::uv::Loop::Mode::kOnce);

  return process;
}

void Connect(nt::NetworkTableInstance nt, nt::BooleanPublisher& kill) {
  nt.SetServer("localhost", nt::NetworkTableInstance::kDefaultPort4);
  nt.StartClient4("localhost");

  kill.Set(false);
  nt.Flush();

  // Wait for NT to connect or fail it if it times out.
  auto time = wpi::Now();
  while (!nt.IsConnected()) {
    // if (wpi::Now() - time > 1.5E7) {
    //   fmt::print(stderr, "The robot program crashed\n");
    //   auto capturedStdout = ::testing::internal::GetCapturedStdout();
    //   auto capturedStderr = ::testing::internal::GetCapturedStderr();
    //   fmt::print(stderr,
    //              "\n******\nRobot Program Captured Output:\n{}\n******\nRobot Program Captured Stderr:\n{}\n******\n",
    //              capturedStdout,
    //              capturedStderr);
    //   std::exit(1);
    // }
  }
}

std::string KillNT(nt::NetworkTableInstance nt, nt::BooleanPublisher& kill) {
  // Before killing sim, store any captured console output.
  auto capturedStdout = ::testing::internal::GetCapturedStdout();

  fmt::print(stderr, "Killing program\n");
  auto time = wpi::Now();

  while (nt.IsConnected()) {
    // Kill program
    kill.Set(true);
    nt.Flush();
    // if (wpi::Now() - time > 3E7) {
    //   EXPECT_TRUE(false);
    //   return capturedStdout;
    // }
  }

  fmt::print(stderr, "Killed robot program\n");

  // Set kill entry to false for future tests
  kill.Set(false);
  nt.Flush();

  // Stop NT Client
  nt.StopClient();

  return capturedStdout;
}
